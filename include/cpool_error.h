#ifndef CPOOL_ERR_H
#define CPOOL_ERR_H

typedef enum cpool_error_t {
  CPOOL_NO_ERR = 0,
  CPOOL_ALLOC_FAILED,
  CPOOL_FAILED_TO_CREATE_THREAD,
  CPOOL_ERR_INIT_MUTEX,
  CPOOL_ERR_INIT_COND,
  CPOOL_INVALID_PARAM,
  CPOOL_NULL_POINTER,
  CPOOL_TASK_QUEUE_FULL,
  CPOOL_INVALID_CONFIG,
} cpool_error_t;

const char *error_to_string(cpool_error_t error);

#endif
