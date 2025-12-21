#ifndef CPOOL_ERR_H
#define CPOOL_ERR_H

enum cpool_error_e
{
  CPOOL_NO_ERR = 0,
  CPOOL_ALLOC_FAILED,
  CPOOL_ERR_INIT_MUTEX,
  CPOOL_ERR_INIT_COND,
  CPOOL_INVALID_PARAM,
  CPOOL_NULL_POINTER,
  CPOOL_TASK_QUEUE_FULL,
  CPOOL_INVALID_CONFIG,
};

const char *error_to_string(enum cpool_error_e error);

#endif
