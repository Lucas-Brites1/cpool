# cpool

**cpool** is a robust, lightweight, and portable Thread Pool library for C.

It provides a simple API to offload tasks to a pool of worker threads, managing synchronization, task queuing, and memory cleanup automatically. It is designed to be leak-free and supports **Graceful Shutdown** via signal handling (SIGINT/SIGTERM).

## ✨ Features

* **Portable:** Works on **Linux**, **macOS**, and **Windows** (MinGW) via a Universal Makefile.
* **Graceful Shutdown:** Intercepts `CTRL+C` to finish pending tasks or stop cleanly without memory corruption.
* **Dynamic Queue:** Handles an unlimited number of tasks (heap-allocated linked list).
* **Thread Safety:** Built-in synchronization using Mutexes and Condition Variables.
* **Zero Leaks:** Rigorous memory management ensures clean exit even on forced interruptions.

---

## 🛠️ Build & Installation

This project uses a **Universal Makefile** that automatically detects your Operating System.

### Prerequisites

* **GCC** (MinGW on Windows, `build-essential` on Linux)
* **Make**

### Compilation Instructions

1.  **Build the static library (`libcpool.a`)**:
    ```bash
    make
    ```

2.  **Build and run the example**:
    ```bash
    make run
    ```

3.  **Clean build artifacts**:
    ```bash
    make clean
    ```

---

## 🚀 Usage Example

Using **cpool** is straightforward. You create a pool, submit tasks, and wait for them to finish.

```c
#include <stdio.h>
#include <stdlib.h>
#include "cpool.h"

// 1. Define your task function
void my_task(void *arg) {
    int *val = (int *)arg;
    printf("Processing item: %d\n", *val);
    free(val); // Don't forget to free the argument if it was malloc'd!
}

int main() {
    // 2. Create a pool with 4 threads and Graceful Shutdown enabled
    cpool_t *pool = cpool_create(4, true);

    // 3. Submit tasks
    for (int i = 0; i < 10; i++) {
        int *arg = malloc(sizeof(int));
        *arg = i;
        cpool_submit(pool, my_task, arg);
    }

    // 4. Wait for all tasks to complete
    cpool_wait(pool);

    // 5. Cleanup resources
    cpool_destroy(pool);

    return 0;
}
```

---

## 📚 API Reference

### `cpool_create`

```c
cpool_t *cpool_create(unsigned int num_threads, bool enable_graceful_shutdown);
```

Initializes the thread pool.

* `num_threads`: Number of worker threads to spawn.
* `enable_graceful_shutdown`: If `true`, captures `SIGINT` (Ctrl+C) to stop processing new tasks and exit cleanly.

### `cpool_submit`

```c
void cpool_submit(cpool_t *pool, cpool_task_func func, void *arg);
```

Adds a task to the queue. The function `func` will be executed by the next available thread.

### `cpool_wait`

```c
void cpool_wait(cpool_t *pool);
```

Blocks the calling thread until all tasks in the queue have been processed.

### `cpool_destroy`

```c
void cpool_destroy(cpool_t *pool);
```

Stops all threads, frees the queue, and destroys mutexes/condition variables. Must be called to avoid memory leaks.

---

## ⚙️ Graceful Shutdown Logic

When `enable_graceful_shutdown` is active:

1. If the user presses `CTRL+C`, a signal handler sets a global stop flag.
2. `cpool_wait()` stops blocking immediately.
3. Worker threads stop picking up new tasks but may finish the current one.
4. Control returns to `main`, allowing `cpool_destroy()` to run and clean up memory safely.

---

## 📂 Project Structure

```
cpool/
├── include/           # Public headers (cpool.h)
├── src/               # Implementation (cpool.c)
├── examples/          # Usage demos
└── Makefile           # Universal Build System
```

---

## 📄 License

This project is open-source. Please check the LICENSE file for details.
