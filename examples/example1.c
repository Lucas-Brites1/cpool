#include "../include/cpool.h"
#include "../include/cpool_error.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void process(void *arg) {
  int id = *((int *)arg);
  pthread_t tid = pthread_self();

  printf("\033[0;32m🟢 [Thread %lu] Starting Task %d\033[0m\n",
         (unsigned long)tid, id);

  sleep(2);

  printf("\033[0;31m🔴 [Thread %lu] END    Task %d\033[0m\n",
         (unsigned long)tid, id);

  free(arg);
}

int main(void) {
  cpool_t *pool = NULL;
  cpool_config_t config;
  cpool_config_init_default(&config);

  config.threads_init_count = 8;
  config.threads_max_count = 16;

  cpool_error_t err = cpool_create(&pool, config);
  if (err != CPOOL_NO_ERR) {
    printf("ERROR: %s\n", error_to_string(err));
    exit(err);
  }

  for (int i = 0; i < 100; i++) {
    int *arg = (int *)malloc(sizeof(int));
    *arg = i;
    cpool_submit(pool, process, arg);
  }

  cpool_wait(pool);
  error_to_string(cpool_destroy(pool));

  return 0;
}
