#ifndef cpool_config_h
#define cpool_config_h
#include <stdbool.h>
#include <stddef.h>

struct cpool_thread_pool_config_t {
  // --- Identity & Logging ---
  char thread_pool_name[64]; // Name for debugging/logging purposes
  char thread_pool_core_threads_name_prefix[32];    // Prefix for permanent
                                                    // (core) threads
  char thread_pool_dynamic_threads_name_prefix[32]; // Prefix for temporary
                                                    // (dynamic) threads
  bool thread_pool_is_log_enable; // Enable internal library logging

  // --- Thread Configuration ---
  size_t threads_init_count; // Number of core threads started initially
  size_t threads_max_count;  // Maximum number of threads allowed (hard limit)

  size_t thread_stack_size; // Stack size per thread in bytes (0 = system
                            // default). Setting this to a lower value (e.g.,
                            // 64KB) saves RAM on high concurrency.

  unsigned int threads_scale_step; // How many dynamic threads to spawn at once
                                   // when load increases
  float threads_scale_threshold;   // Load percentage (0.0 to 1.0) required to
                                   // trigger scaling

  bool threads_kill_dynamic_thread_when_exceed_time; // If true, kills dynamic
                                                     // threads that are idle
                                                     // for too long
  size_t threads_kill_time_ms; // Idle time in milliseconds before killing a
                               // dynamic thread

  // --- Task Configuration (Object Pool & Queue) ---
  size_t tasks_init_count;       // Initial number of pre-allocated task objects
  size_t tasks_max_count;        // Maximum number of pending tasks in the queue
  size_t tasks_chunk_block_size; // Number of tasks to allocate per new memory
                                 // chunk

  float tasks_scale_threshold;    // Threshold (0.0 to 1.0) of used tasks to
                                  // trigger new chunk allocation
  size_t tasks_time_limit_to_log; // Threshold in milliseconds to log a warning
                                  // if a task takes too long

  // --- Shutdown Policy ---
  bool cpool_is_enabled_graceful_shutdown; // If true, waits for pending tasks
                                           // to finish before destroying the
                                           // pool. If false, tries to stop
                                           // immediately.
};

#define KiB (1024UL)
#define MiB (1024UL * KiB)

#define CPOOL_DEFAULT_STACK_SIZE 0 // Uses OS default (usually 2MB-8MB)
#define CPOOL_STACK_SIZE_64KB (64 * KiB)
#define CPOOL_STACK_SIZE_128KB (128 * KiB)
#define CPOOL_STACK_SIZE_256KB (256 * KiB)
#define CPOOL_STACK_SIZE_512KB (512 * KiB)
#define CPOOL_STACK_SIZE_1MB (1 * MiB)
#define CPOOL_STACK_SIZE_2MB (2 * MiB)
#define CPOOL_STACK_SIZE_4MB (4 * MiB)
#define CPOOL_STACK_SIZE_8MB (8 * MiB)
#define CPOOL_STACK_SIZE_16MB (16 * MiB)

#define CPOOL_CPU_AUTO_DETECT 0
#define CPOOL_DEFAULT_KILL_TIME_MS 10000

#endif // !CPOOL_CONFIG_H
