#include "../include/cpool_error.h"

const char *error_to_string(cpool_error_t error) {
  switch (error) {
  // --- Success ---
  case CPOOL_NO_ERR:
    return "Operation completed successfully";

  // --- Runtime Errors ---
  case CPOOL_ALLOC_FAILED:
    return "Failed to allocate memory (internal allocation error)";
  case CPOOL_ERR_INIT_MUTEX:
    return "Failed to initialize the sync mutex";
  case CPOOL_ERR_INIT_COND:
    return "Failed to initialize the condition variable";
  case CPOOL_FAILED_TO_CREATE_THREAD:
    return "Operating System failed to create a new thread";
  case CPOOL_INVALID_PARAM:
    return "One or more parameters are invalid or out of range";
  case CPOOL_NULL_POINTER:
    return "Unexpected null pointer argument provided";
  case CPOOL_TASK_QUEUE_FULL:
    return "Task queue has reached maximum capacity - cannot add new item";
  case CPOOL_SHUTDOWN_IN_PROGRESS:
    return "Operation rejected: The pool is currently shutting down";

  // --- Configuration Errors (Validation) ---
  case CPOOL_CONF_MISSING:
    return "Configuration Error: The config object passed is NULL";

  // Threads Config
  case CPOOL_CONF_NO_THREADS:
    return "Configuration Error: 'threads_max_count' must be greater than 0";
  case CPOOL_CONF_INIT_THREADS_GT_MAX:
    return "Configuration Error: 'threads_init_count' cannot be greater than "
           "'threads_max_count'";
  case CPOOL_CONF_STACK_SIZE_TOO_SMALL:
    return "Configuration Error: 'thread_stack_size' is below the system "
           "minimum (PTHREAD_STACK_MIN)";

  // Tasks Config
  case CPOOL_CONF_NO_TASKS:
    return "Configuration Error: 'tasks_max_count' must be greater than 0";
  case CPOOL_CONF_INIT_TASKS_GT_MAX:
    return "Configuration Error: 'tasks_init_count' cannot be greater than "
           "'tasks_max_count'";
  case CPOOL_CONF_BLOCK_SIZE_ZERO:
    return "Configuration Error: 'tasks_chunk_block_size' cannot be 0";

  // Scaling Config
  case CPOOL_CONF_INVALID_THRESHOLD:
    return "Configuration Error: Scaling thresholds must be between 0.0 and "
           "1.0";

  // Fallback
  case CPOOL_INVALID_CONFIG:
    return "The configuration structure is invalid or incomplete (Generic "
           "error)";

  default:
    return "Unknown error code";
  }
}
