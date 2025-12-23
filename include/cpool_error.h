#ifndef CPOOL_ERR_H
#define CPOOL_ERR_H

typedef enum cpool_error_t {
  CPOOL_NO_ERR = 0,

  // --- Runtime Errors ---
  CPOOL_ALLOC_FAILED,
  CPOOL_ERR_INIT_MUTEX,
  CPOOL_ERR_INIT_COND,
  CPOOL_ERR_FREE_MUTEX,
  CPOOL_ERR_FREE_COND,
  CPOOL_FAILED_TO_CREATE_THREAD,
  CPOOL_NULL_POINTER,
  CPOOL_TASK_QUEUE_FULL,
  CPOOL_SHUTDOWN_IN_PROGRESS,
  CPOOL_INVALID_PARAM,
  CPOOL_NULL_FREE_TASK,

  // --- Configuration Errors (Threads) ---
  CPOOL_CONF_MISSING,              // conf == NULL
  CPOOL_CONF_NO_THREADS,           // max_threads == 0
  CPOOL_CONF_INIT_THREADS_GT_MAX,  // init_threads > max_threads
  CPOOL_CONF_STACK_SIZE_TOO_SMALL, // stack_size < PTHREAD_STACK_MIN

  // --- Configuration Errors (Tasks) ---
  CPOOL_CONF_NO_TASKS,          // max_tasks == 0
  CPOOL_CONF_INIT_TASKS_GT_MAX, // init_tasks > max_tasks
  CPOOL_CONF_BLOCK_SIZE_ZERO,   // chunk_block_size == 0

  // --- Configuration Errors (Scaling) ---
  CPOOL_CONF_INVALID_THRESHOLD, // threshold < 0.0 ou > 1.0

  // Fallbak Generic Error
  CPOOL_INVALID_CONFIG,
} cpool_error_t;

const char *error_to_string(cpool_error_t error);

#endif
