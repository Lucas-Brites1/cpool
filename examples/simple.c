/**
 * Example: Simple usage of the CPool library.
 * Demonstrates submitting tasks, atomic counters, and graceful shutdown.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "../include/cpool.h"

#define NUM_THREADS 6
#define TOTAL_TASKS 500

// Volatile variable to prevent compiler optimization on reads/writes
static volatile int completed_tasks = 0;

/**
 * Worker function that simulates a heavy workload.
 * It sleeps for a random time and increments the global counter atomically.
 */
void task_handler(void *arg)
{
  int task_id = *((int *)arg);

  // Print current thread ID and task ID for debugging purposes
  printf("[Thread %lu] Processing task ID: %d\n", pthread_self(), task_id);

  // Simulate workload (0 to 100ms)
  usleep(1000 * (rand() % 100));

  // Atomic increment to ensure thread safety without explicit mutexes here
  __sync_fetch_and_add(&completed_tasks, 1);

  // IMPORTANT: Free the memory allocated by the main thread
  free(arg);
}

int main(void)
{
  printf("--- Starting CPool Example ---\n");

  // Initialize the thread pool with graceful shutdown enabled
  cpool_t *pool = cpool_create(NUM_THREADS, true);
  if (!pool)
  {
    fprintf(stderr, "Failed to create thread pool.\n");
    return 1;
  }

  printf("Dispatching %d tasks to %d threads...\n", TOTAL_TASKS, NUM_THREADS);

  for (int i = 0; i < TOTAL_TASKS; i++)
  {
    // Allocate memory for the argument to avoid race conditions
    int *arg = (int *)malloc(sizeof(int));
    *arg = i;

    cpool_submit(pool, task_handler, arg);
  }

  // Wait for all tasks to complete and then clean up resources
  cpool_wait(pool);
  cpool_destroy(pool);

  printf("--- Finished ---\n");
  printf("Total tasks processed: %d / %d\n", completed_tasks, TOTAL_TASKS);

  return 0;
}
