#ifndef CPOOL_CHUNK_H
#define CPOOL_CHUNK_H
#include "cpool_task.h"
#include "cpool_types.h"

struct cpool_chunk_t {
  cpool_task_t *tasks;
  size_t size;

  struct cpool_chunk_t *next;
};

#endif // !CPOOL_CHUNK_H
