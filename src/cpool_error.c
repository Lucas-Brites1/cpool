#include <stdio.h>
#include <stdlib.h>
#include "cpool_error.h"

const char *error_to_string(enum cpool_error_e error)
{
  switch (error)
  {
  case CPOOL_NO_ERR:
    return "Operation completed successfully";
  case CPOOL_ALLOC_FAILED:
    return "Failed to allocate memory (internal allocation error)";
  case CPOOL_ERR_INIT_MUTEX:
    return "Failed to initialize the sync mutex";
  case CPOOL_ERR_INIT_COND:
    return "Failed to initialize the condition variable";
  case CPOOL_INVALID_PARAM:
    return "One or more parameters are invalid or out of range";
  case CPOOL_NULL_POINTER:
    return "Unexpected null pointer argument provided";
  case CPOOL_TASK_QUEUE_FULL:
    return "Task queue has reached maximum capacity - cannot add new item";
  case CPOOL_INVALID_CONFIG:
    return "The configuration structure is invalid or incomplete";
  default:
    return "Unkown error";
  }
}
