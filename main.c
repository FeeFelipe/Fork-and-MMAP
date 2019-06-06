#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <sys/wait.h>
#include <sys/mman.h>

// Defines a task of integration over the quarter of circle, with an x-axis
// start and end indexes. The work must be done in the interval [start, end[
struct task {
  int start, end;
};

#define DIE(...) { \
  fprintf(stderr, __VA_ARGS__); \
  exit(EXIT_FAILURE); \
}

void *shared_malloc(size_t len) {
  int protection = PROT_READ | PROT_WRITE;
  int visibility = MAP_ANONYMOUS | MAP_SHARED;

  void *p = mmap(0, len, protection, visibility, -1, 0);

  if (p==(void *)-1) printf("mmap failed!\n");
  return p;
}

void shared_free(void *p,int len) {
  munmap(p,len);
}

void process_work(struct task *t, double *shared_memory, double NUM_PONTOS) {
  double acc = 0;
  double interval_size = 1.0 / NUM_PONTOS;

  // Integrates f(x) = sqrt(1 - x^2) in [t->start, t->end[
  for(int i = t->start; i < t->end; ++i) {
    double x = (i * interval_size) + interval_size / 2;
    acc += sqrt(1 - (x * x)) * interval_size;
  }

  *shared_memory += acc;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    DIE("./pi_process NUM_PROCESSOS NUM_PONTOS\n");
    return 1;
  }

  int NUM_PROCESSOS = atoi(argv[1]);
  int NUM_PONTOS = atoi(argv[2]);
  struct task *tasks;

  if((tasks = malloc(NUM_PROCESSOS * sizeof(struct task))) == NULL)
    DIE("Tasks malloc failed\n");

  double *shared_memory = shared_malloc(128);
  pid_t pid[NUM_PROCESSOS];

  int process_with_one_more_work = NUM_PONTOS % NUM_PROCESSOS;
  for (int i = 0; i < NUM_PROCESSOS; ++i) {
    int work_size = NUM_PONTOS / NUM_PROCESSOS;

    if (i < process_with_one_more_work)
      work_size += 1;

    tasks[i].start = i * work_size;
    tasks[i].end = (i + 1) * work_size;

    pid[i]=fork();
    if (pid[i]==0) {
      process_work(&tasks[i], shared_memory, NUM_PONTOS);
      exit(0);
    } else if (pid[i] < 0)
      DIE("Failed to create thread %d\n", i)
  }

  for (int i=0; i < NUM_PROCESSOS; i++) {
    int ret;
    waitpid(pid[i],&ret,0);
  }

  printf("pi ~= %.12f\n", *shared_memory * 4);

  shared_free(shared_memory, sizeof *shared_memory);

  return 0;
}
