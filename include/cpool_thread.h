#ifndef CPOOL_THREAD_H
#define CPOOL_THREAD_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#define CPOOL_ATOMIC_BOOL atomic_bool
#define CPOOL_ATOMIC_STORE(ptr, val) atomic_store(ptr, val)
#define CPOOL_ATOMIC_LOAD(ptr) atomic_load(ptr)

#elif defined(__GNUC__)
// --- MODO GCC LEGADO (O Pulo do Gato) ---
// GCC antigo não tem stdatomic.h, mas tem __sync functions!
// Isso gera as instruções de barreira no Assembly (seguro pra ARM)

#define CPOOL_ATOMIC_BOOL volatile bool

// __sync_synchronize() cria a barreira de memória que faltava
#define CPOOL_ATOMIC_STORE(ptr, val) \
  do                                 \
  {                                  \
    *(ptr) = (val);                  \
    __sync_synchronize();            \
  } while (0)
#define CPOOL_ATOMIC_LOAD(ptr) (*(ptr))

#else
#define CPOOL_ATOMIC_BOOL volatile bool
#define CPOOL_ATOMIC_STORE(ptr, val) (*(ptr) = (val))
#define CPOOL_ATOMIC_LOAD(ptr) (*(ptr))
#endif

#endif
