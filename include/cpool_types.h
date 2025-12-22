#ifndef CPOOL_TYPES_H
#define CPOOL_TYPES_H

#include "cpool_error.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct cpool_thread_pool_config_t cpool_config_t;
typedef struct cpool_worker_t cpool_worker_t;
typedef struct cpool_task_t cpool_task_t;
typedef struct cpool_chunk_t cpool_chunk_t;
typedef struct cpool_t cpool_t;

typedef void (*worker_routine_func)(void *arg);

#endif // CPOOL_TYPES_H
