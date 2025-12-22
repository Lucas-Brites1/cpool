#ifndef CPOOL_THREAD_H
#define CPOOL_THREAD_H

#include "cpool_types.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

struct cpool_worker_t {
  cpool_t *pool;
  char worker_buffer_name[64];
  unsigned int worker_id;
  pthread_t tid;

  bool is_core;
  bool is_processing;
  bool should_stop;

  uint64_t last_active_time;
};

#endif
