#ifndef CPOOL_TASK_H
#define CPOOL_TASK_H

#include "cpool_types.h"

struct cpool_task_t {
  worker_routine_func func;
  void *arg;
  cpool_task_t *next;
};

#endif // !CPOOL_TASK_H
