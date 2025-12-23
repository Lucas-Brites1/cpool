#ifndef CPOOL_H
#define CPOOL_H

#include "cpool_config.h"
#include "cpool_types.h"

void cpool_config_init_default(cpool_config_t *conf);
cpool_error_t cpool_create(cpool_t **pool_out, cpool_config_t config);
cpool_error_t cpool_submit(cpool_t *thread_pool, worker_routine_func func,
                           void *arg);
cpool_error_t cpool_wait(cpool_t *thread_pool);
cpool_error_t cpool_destroy(cpool_t *thread_pool);

#endif
