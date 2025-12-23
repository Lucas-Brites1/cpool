#include "../include/cpool.h"
#include "../include/cpool_chunk.h"
#include "../include/cpool_task.h"
#include "../include/cpool_worker.h"
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct cpool_t {
  cpool_config_t conf;

  cpool_worker_t *workers;

  cpool_chunk_t *tasks_chunks;
  size_t next_chunk_size;
  size_t task_total_allocated;

  cpool_task_t *free_head; // object pool
  cpool_task_t *queue_head;
  cpool_task_t *queue_tail;

  size_t pending_tasks_to_complete;
  size_t current_tasks_being_processed;

  pthread_mutex_t mtx;
  pthread_cond_t notify;
  pthread_cond_t finish;
} cpool_t;

// Config functions
static cpool_error_t cpool__sanitize_config(cpool_config_t *conf);

// Helpers
static cpool_error_t cpool__alloc(size_t quantity, size_t size,
                                  void **ptr); // for object pool
static cpool_error_t cpool__initialize_sync(cpool_t *pool);
static cpool_error_t cpool__internal_destroy_sync(cpool_t *pool);
static void cpool__mutex_lock(cpool_t *pool);
static void cpool__mutex_unlock(cpool_t *pool);
static void cpool__cond_alert(pthread_cond_t *cond);
static void cpool__cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx);
static pthread_attr_t cpool__get_stacksize_attr_by_conf(cpool_config_t *conf);

// Chunk tasks && Free Head functions
static inline size_t cpool__calculate_new_chunk_tasks_block_size(cpool_t *pool);
static cpool_error_t cpool__internal_destroy_chunks(cpool_t *pool);
static cpool_chunk_t *cpool__new_chunk_task(cpool_t *pool, size_t count);
static cpool_error_t cpool__merge_chunk_to_list(cpool_chunk_t *chunk,
                                                cpool_task_t **head_ref);
static inline bool cpool__should_alloc_new_chunk(cpool_t *pool);

// Queue functions
static cpool_task_t *cpool__get_free_task(cpool_t *pool);     // 1
static cpool_error_t cpool_tasks_queue_enqueue(cpool_t *pool, // 2
                                               cpool_task_t *task);
static cpool_task_t *cpool_tasks_queue_dequeue(cpool_t *pool); // 3
static void cpool__execute_task(cpool_task_t *task);           // 4

// Workers functions
static cpool_error_t cpool__spawn_core_workers(cpool_t *pool);
static void cpool__set_worker_name(cpool_worker_t *self, cpool_config_t *conf);
static inline bool cpool__worker_should_scale_up(cpool_t *pool);
static inline bool cpool__worker_should_scale_down(cpool_t *pool,
                                                   cpool_worker_t *self);
static cpool_error_t cpool__worker_exitr_scale_up(cpool_t *pool);
static void cpool__worker_exit(cpool_t *pool, cpool_worker_t *worker);
static void *cpool_worker_routine_loop(void *arg);

static cpool_error_t cpool__alloc(size_t quantity, size_t size, void **ptr) {
  if (!ptr) {
    return CPOOL_NULL_POINTER;
  }

  void *block = calloc(quantity, size);
  if (!block) {
    return CPOOL_ALLOC_FAILED;
  }

  *ptr = block;
  return CPOOL_NO_ERR;
}

static void cpool__mutex_lock(cpool_t *pool) { pthread_mutex_lock(&pool->mtx); }

static void cpool__mutex_unlock(cpool_t *pool) {
  pthread_mutex_unlock(&pool->mtx);
}

static void cpool__cond_alert(pthread_cond_t *cond) {
  pthread_cond_signal(cond);
}

static void cpool__cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx) {
  pthread_cond_wait(cond, mtx);
}

static cpool_error_t cpool__initialize_sync(cpool_t *pool) {
  int rc = pthread_mutex_init(&pool->mtx, NULL);
  if (rc != 0) {
    return CPOOL_ERR_INIT_MUTEX;
  }

  rc = pthread_cond_init(&pool->notify, NULL);
  if (rc != 0) {
    return CPOOL_ERR_INIT_COND;
  }

  rc = pthread_cond_init(&pool->notify, NULL);
  if (rc != 0) {
    return CPOOL_ERR_INIT_COND;
  }

  return CPOOL_NO_ERR;
}

static cpool_error_t cpool__internal_destroy_sync(cpool_t *pool) {
  int rc = pthread_mutex_destroy(&pool->mtx);
  if (rc != 0) {
    return CPOOL_ERR_FREE_MUTEX;
  }

  rc = pthread_cond_destroy(&pool->notify);
  if (rc != 0) {
    return CPOOL_ERR_FREE_COND;
  }

  rc = pthread_cond_destroy(&pool->finish);
  if (rc != 0) {
    return CPOOL_ERR_FREE_COND;
  }

  return CPOOL_NO_ERR;
}

static pthread_attr_t cpool__get_stacksize_attr_by_conf(cpool_config_t *conf) {
  pthread_attr_t attr = {};
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, conf->thread_stack_size);
  return attr;
}

static inline size_t
cpool__calculate_new_chunk_tasks_block_size(cpool_t *pool) {
  size_t space_remaining =
      pool->conf.tasks_max_count - pool->task_total_allocated;

  if (space_remaining == 0) {
    return 0;
  }

  size_t size_to_alloc = pool->conf.tasks_chunk_block_size;

  if (size_to_alloc > space_remaining) {
    size_to_alloc = space_remaining;
  }

  if (pool->task_total_allocated >= pool->conf.tasks_max_count) {
    pool->next_chunk_size = 0;
  } else {
    pool->next_chunk_size = pool->conf.tasks_chunk_block_size;
  }

  return size_to_alloc;
}

static inline bool cpool__should_alloc_new_chunk(cpool_t *pool) {
  if (pool->task_total_allocated >= pool->conf.tasks_max_count) {
    return false;
  }

  if (pool->task_total_allocated == 0) {
    return true;
  }

  size_t used_tasks =
      pool->current_tasks_being_processed + pool->pending_tasks_to_complete;

  size_t capacity = pool->task_total_allocated;
  size_t threshold_trigger =
      (size_t)(capacity * pool->conf.tasks_scale_threshold);

  return used_tasks >= threshold_trigger;
}

static cpool_chunk_t *cpool__new_chunk_task(cpool_t *pool, size_t count) {
  if (!pool) {
    return NULL;
  }

  if (count == 0) {
    return NULL;
  }

  size_t total_size = sizeof(cpool_chunk_t) + (count * sizeof(cpool_task_t));
  void *ptr_to_alloc = NULL;
  cpool__alloc(1, total_size, &ptr_to_alloc);
  if (!ptr_to_alloc) {
    return NULL;
  }

  cpool_chunk_t *new_chunk = (cpool_chunk_t *)ptr_to_alloc;
  new_chunk->size = count;
  new_chunk->next = NULL;

  new_chunk->tasks = (cpool_task_t *)(new_chunk + 1);

  for (size_t i = 0; i < count - 1; i++) {
    new_chunk->tasks[i].next = &new_chunk->tasks[i + 1];
  }

  new_chunk->tasks[count - 1].next = NULL;

  pool->task_total_allocated += count;
  return new_chunk;
}

static cpool_error_t cpool__merge_chunk_to_list(cpool_chunk_t *chunk,
                                                cpool_task_t **head_ref) {
  if (!chunk || !head_ref) {
    return CPOOL_NULL_POINTER;
  }

  chunk->tasks[chunk->size - 1].next = *head_ref;

  *head_ref = chunk->tasks;

  return CPOOL_NO_ERR;
}

static void cpool__set_worker_name(cpool_worker_t *self, cpool_config_t *conf) {
  size_t sizeof_buffer = sizeof(self->worker_buffer_name);
  bool is_core = self->is_core;

  char *target_prefix = is_core ? conf->thread_pool_core_threads_name_prefix
                                : conf->thread_pool_dynamic_threads_name_prefix;

  if (target_prefix[0] != '\0') {
    snprintf(self->worker_buffer_name, sizeof_buffer, "%s-%u", target_prefix,
             self->worker_id);
  } else {
    snprintf(self->worker_buffer_name, sizeof_buffer, "cpool-%s-%u",
             is_core ? "core" : "dyn", self->worker_id);
  }

  size_t current_len = strlen(self->worker_buffer_name);
  if (current_len < sizeof_buffer) {
    snprintf(self->worker_buffer_name + current_len,
             sizeof_buffer - current_len, "-tid-%lu", (unsigned long)self->tid);
  }
}

static cpool_error_t cpool__spawn_core_workers(cpool_t *pool) {
  if (!pool) {
    return CPOOL_NULL_POINTER;
  }

  size_t core_workers_count = pool->conf.threads_init_count;
  size_t total_workers_count = pool->conf.threads_max_count;

  void *ptr_to_alloc = NULL;
  cpool_error_t err =
      cpool__alloc(total_workers_count, sizeof(cpool_worker_t), &ptr_to_alloc);

  if (err != CPOOL_NO_ERR || !ptr_to_alloc) {
    return CPOOL_ALLOC_FAILED;
  }

  cpool_worker_t *workers = (cpool_worker_t *)ptr_to_alloc;

  pool->workers = workers;

  pthread_attr_t attr = cpool__get_stacksize_attr_by_conf(&pool->conf);

  for (size_t i = 0; i < core_workers_count; i++) {
    cpool_worker_t *worker = &pool->workers[i];

    worker->pool = pool;
    worker->worker_id = (unsigned int)i;
    worker->is_core = true;
    int rc =
        pthread_create(&worker->tid, &attr, cpool_worker_routine_loop, worker);
    if (rc != 0) {
      for (size_t j = 0; j < i; j++) {
        pthread_cancel(pool->workers[j].tid);
        pthread_join(pool->workers[j].tid, NULL);
      }
      pthread_attr_destroy(&attr);
      // cpool__internal_destroy(pool);
      return CPOOL_FAILED_TO_CREATE_THREAD;
    }

    cpool__set_worker_name(worker, &pool->conf);
  }

  pthread_attr_destroy(&attr);

  return CPOOL_NO_ERR;
}

static cpool_error_t cpool__sanitize_config(cpool_config_t *conf) {
  if (!conf) {
    return CPOOL_CONF_MISSING;
  }

  if (conf->threads_max_count == 0) {
    return CPOOL_CONF_NO_THREADS;
  }

  if (conf->threads_init_count > conf->threads_max_count) {
    return CPOOL_CONF_INIT_THREADS_GT_MAX;
  }

  if (conf->thread_stack_size != 0 &&
      conf->thread_stack_size < PTHREAD_STACK_MIN) {
    fprintf(stderr,
            "[CPOOL] Error: Stack size %zu is too small. Min required: %zu\n",
            conf->thread_stack_size, (size_t)PTHREAD_STACK_MIN);
    return CPOOL_CONF_STACK_SIZE_TOO_SMALL;
  }

  if (conf->tasks_max_count == 0) {
    return CPOOL_CONF_NO_TASKS;
  }

  if (conf->tasks_init_count > conf->tasks_max_count) {
    return CPOOL_CONF_INIT_TASKS_GT_MAX;
  }

  if (conf->tasks_chunk_block_size == 0) {
    size_t calc = conf->tasks_max_count / 4;
    conf->tasks_chunk_block_size = (calc > 8) ? calc : 8;
  }

  if (conf->threads_scale_step == 0) {
    conf->threads_scale_step = 1;
  }

  if (conf->threads_scale_threshold <= 0.001f ||
      conf->threads_scale_threshold > 1.0f) {
    conf->threads_scale_threshold = 0.75f;
  }

  if (conf->threads_kill_dynamic_thread_when_exceed_time &&
      conf->threads_kill_time_ms < 100) {
    conf->threads_kill_time_ms = CPOOL_DEFAULT_KILL_TIME_MS;
  }

  return CPOOL_NO_ERR;
}

void cpool_config_init_default(cpool_config_t *conf) {
  if (!conf)
    return;

  memset(conf, 0, sizeof(cpool_config_t));

  long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
  num_cores = num_cores <= 1 ? 2 : num_cores;

  // Threads Configs
  conf->threads_init_count = (size_t)num_cores;
  conf->threads_max_count = (size_t)num_cores * 2;
  conf->thread_stack_size = CPOOL_DEFAULT_STACK_SIZE;
  conf->threads_scale_threshold = 0.75f;
  conf->threads_scale_step = 2;

  // Tasks Configs
  size_t start_tasks = (size_t)num_cores * 16;
  conf->tasks_chunk_block_size = 32;
  conf->tasks_init_count = (start_tasks < 64) ? 64 : start_tasks;
  conf->tasks_max_count = 4096;
  conf->tasks_scale_threshold = 0.75f;

  // Policies Configs
  conf->cpool_is_enabled_graceful_shutdown = true;
  conf->threads_kill_dynamic_thread_when_exceed_time = true;
  conf->threads_kill_time_ms = CPOOL_DEFAULT_KILL_TIME_MS;

  // Identity
  snprintf(conf->thread_pool_name, sizeof(conf->thread_pool_name),
           "default-pool");
}

static cpool_task_t *cpool__get_free_task(cpool_t *pool) {
  if (!pool->free_head) {
    if (!cpool__should_alloc_new_chunk(pool)) {
      return NULL;
    }

    cpool_chunk_t *new_chunk =
        cpool__new_chunk_task(pool, pool->next_chunk_size);

    if (!new_chunk) {
      return NULL;
    }

    cpool__merge_chunk_to_list(new_chunk, &pool->free_head);
  }

  cpool_task_t *task = pool->free_head;
  pool->free_head = task->next;
  task->next = NULL;

  return task;
}

static void *cpool_worker_routine_loop(void *arg) {
  cpool_worker_t *worker = (cpool_worker_t *)arg;
  if (!worker) {
    return NULL;
  }

  cpool_t *pool = worker->pool;
  printf("Hello from worker:  %s\n", worker->worker_buffer_name);
  return NULL;
}

static cpool_error_t cpool_tasks_queue_enqueue(cpool_t *pool, // 2
                                               cpool_task_t *task) {
  if (!pool || !task) {
    return CPOOL_NULL_POINTER;
  }

  task->next = NULL;

  if (!pool->queue_head) {
    pool->queue_head = task;
    pool->queue_tail = task;
  } else {
    pool->queue_tail->next = task;
    pool->queue_tail = task;
  }

  pool->pending_tasks_to_complete++;

  return CPOOL_NO_ERR;
}

static cpool_task_t *cpool_tasks_queue_dequeue(cpool_t *pool) {
  if (!pool || !pool->queue_head) {
    return NULL;
  }

  cpool_task_t *t = pool->queue_head;
  pool->queue_head = pool->queue_head->next;

  if (!pool->queue_head) {
    pool->queue_tail = NULL;
  }

  pool->pending_tasks_to_complete--;
  pool->current_tasks_being_processed++;

  t->next = NULL;
  return t;
}

static void cpool__execute_task(cpool_task_t *task) {
  if (!task || !task->func) {
    return;
  }

  task->func(task->arg);
}
