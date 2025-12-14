#ifndef CPOOL_H
#define CPOOL_H

#include <stdbool.h>

typedef void (*cpool_task_func)(void *arg);
typedef struct cpool cpool_t;

cpool_t *cpool_create(unsigned int num_threads, bool enable_graceful_shutdown);
void cpool_submit(cpool_t *pool, cpool_task_func func, void *arg);
void cpool_wait(cpool_t *pool);
void cpool_destroy(cpool_t *pool);

#endif
