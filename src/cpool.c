#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include "../include/cpool.h"

typedef struct task
{
  cpool_task_func function;
  void *arg;
  struct task *next;
} Task;

struct cpool
{
  Task *head;
  Task *tail;

  pthread_t *threads;
  size_t num_threads;

  bool stop;
  pthread_mutex_t mutex;
  pthread_cond_t notify;
  pthread_cond_t finished;

  unsigned int task_count;
};

static cpool_t *g_pool = NULL;

static bool cpool_task_list_is_empty(cpool_t *p);
static void cpool_execute_task(Task *t);
static Task *pop_from_queue(cpool_t *p);
static void insert_at_queue(cpool_t *p, Task *t);
static int cpool_init_threads(cpool_t *p);
static void *cpool_thread_worker_function(void *arg);
static void cpool_signal_handler(int sig);
static void cpool_task_append(cpool_t *pool, Task *task);
static void cpool_free_task_queue(cpool_t *p);

cpool_t *cpool_create(unsigned int num_threads, bool enable_graceful_shutdown)
{
  if (num_threads == 0)
  {
    return NULL;
  }

  cpool_t *pool = (cpool_t *)malloc(sizeof(cpool_t));
  if (!pool)
  {
    return NULL;
  }

  pool->num_threads = num_threads;
  pool->head = NULL;
  pool->tail = NULL;
  pool->stop = false;
  pool->task_count = 0;

  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->notify, NULL);
  pthread_cond_init(&pool->finished, NULL);

  if (enable_graceful_shutdown)
  {
    g_pool = pool;
    signal(SIGINT, cpool_signal_handler);
    signal(SIGTERM, cpool_signal_handler);
  }

  if (cpool_init_threads(pool) != 0)
  {
    free(pool);
    return NULL;
  }

  return pool;
}

void cpool_destroy(cpool_t *pool)
{
  if (!pool)
    return;

  pthread_mutex_lock(&pool->mutex);
  pool->stop = true;
  pthread_cond_broadcast(&pool->notify);
  pthread_mutex_unlock(&pool->mutex);

  for (size_t i = 0; i < pool->num_threads; i++)
  {
    pthread_join(pool->threads[i], NULL);
  }

  cpool_free_task_queue(pool);

  pthread_mutex_destroy(&pool->mutex);
  pthread_cond_destroy(&pool->notify);
  pthread_cond_destroy(&pool->finished);
  free(pool->threads);
  free(pool);
}

void cpool_submit(cpool_t *pool, cpool_task_func func, void *arg)
{
  if (!pool || !func)
    return;

  Task *t = malloc(sizeof(Task));
  if (!t)
    return;

  t->next = NULL;
  t->function = func;
  t->arg = arg;
  cpool_task_append(pool, t);
}

void cpool_wait(cpool_t *pool)
{
  if (!pool)
    return;

  pthread_mutex_lock(&pool->mutex);

  while (pool->task_count > 0 && !pool->stop)
  {
    pthread_cond_wait(&pool->finished, &pool->mutex);
  }
  pthread_mutex_unlock(&pool->mutex);
}

static void cpool_task_append(cpool_t *pool, Task *task)
{
  if (!pool || !task)
    return;

  pthread_mutex_lock(&pool->mutex);
  insert_at_queue(pool, task);
  pool->task_count++;
  pthread_cond_signal(&pool->notify);
  pthread_mutex_unlock(&pool->mutex);
}

static int cpool_init_threads(cpool_t *p)
{
  p->threads = (pthread_t *)malloc(sizeof(pthread_t) * p->num_threads);
  if (!p->threads)
    return -1;

  for (size_t i = 0; i < p->num_threads; i++)
  {
    pthread_create(&p->threads[i], NULL, cpool_thread_worker_function, p);
  }

  return 0;
}

static void *cpool_thread_worker_function(void *arg)
{
  cpool_t *pool = (cpool_t *)arg;

  while (1)
  {
    pthread_mutex_lock(&pool->mutex);

    while (cpool_task_list_is_empty(pool) && !pool->stop)
    {
      pthread_cond_wait(&pool->notify, &pool->mutex);
    }

    if (pool->stop)
    {
      pthread_mutex_unlock(&pool->mutex);
      break;
    }

    Task *task = pop_from_queue(pool);
    pthread_mutex_unlock(&pool->mutex);

    cpool_execute_task(task);
    free(task);

    pthread_mutex_lock(&pool->mutex);
    pool->task_count--;

    if (pool->task_count == 0 || pool->stop)
    {
      pthread_cond_signal(&pool->finished);
    }
    pthread_mutex_unlock(&pool->mutex);
  }

  return NULL;
}

static void insert_at_queue(cpool_t *p, Task *t)
{
  if (!p || !t)
    return;

  t->next = NULL;

  if (!p->head)
  {
    p->head = t;
    p->tail = t;
    return;
  }

  p->tail->next = t;
  p->tail = t;
}

static Task *pop_from_queue(cpool_t *p)
{
  if (!p || !p->head)
    return NULL;

  Task *q_head = p->head;
  p->head = p->head->next;

  if (!p->head)
  {
    p->tail = NULL;
  }

  q_head->next = NULL;
  return q_head;
}

static void cpool_execute_task(Task *t)
{
  if (!t)
    return;

  t->function(t->arg);
}

static bool cpool_task_list_is_empty(cpool_t *p)
{
  return p->head == NULL;
}

static void cpool_signal_handler(int sig)
{
  (void)sig;
  if (g_pool)
  {
    g_pool->stop = true;

    pthread_cond_broadcast(&g_pool->notify);

    pthread_cond_broadcast(&g_pool->finished);
  }
}

static void cpool_free_task_queue(cpool_t *p)
{
  if (!p || !p->head)
    return;

  Task *current = p->head;
  while (current != NULL)
  {
    Task *next = current->next;
    free(current);
    current = next;
  }

  p->head = NULL;
  p->tail = NULL;
}
