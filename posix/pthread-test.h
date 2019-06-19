#ifndef POSIX_PTHREAD_H
#define POSIX_PTHREAD_H
#define _SYS__PTHREADTYPES_H_
#include "stdbool.h"
#include "cmsis_os2.h"
#include "mbed_rtos1_types.h"
#include "mbed_rtos_storage.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>

#define PTHREAD_PRIO_INHERIT osMutexPrioInherit

#ifdef __cplusplus
extern "C" {
#endif

typedef osMutexAttr_t pthread_mutexattr_t;
typedef osSemaphoreAttr_t pthread_condattr_t;

typedef struct pthread_mutex_t
{
  osMutexId_t _id;
  mbed_rtos_storage_mutex_t _obj_mem;
}pthread_mutex_t;

typedef struct Waiter
{
  osSemaphoreId_t sem;
  pthread_mutex_t _mutex;
  bool in_list;
  struct Waiter * prev;
  struct Waiter * next;
  struct Waiter * _wait_list;
}Waiter;

typedef struct pthread_cond_t
{
  osSemaphoreId_t _id;
  mbed_rtos_storage_semaphore_t _obj_mem;
  Waiter waiter;
}pthread_cond_t;


extern int pthread_mutex_init (pthread_mutex_t *__mutex, pthread_mutexattr_t *__mutexattr);
extern int pthread_mutex_lock (pthread_mutex_t * __mutex);
extern int pthread_mutex_unlock (pthread_mutex_t * __mutex);
extern int pthread_mutexattr_setprotocol (pthread_mutexattr_t *__attr, int __protocol);
extern int pthread_mutex_destroy (pthread_mutex_t * __mutex);

extern int pthread_cond_init (pthread_cond_t *__restrict __cond, pthread_condattr_t *__restrict __cond_attr);
extern int pthread_cond_destroy (pthread_cond_t *__cond);
extern int pthread_cond_signal (pthread_cond_t *__cond);
extern int pthread_cond_broadcast (pthread_cond_t *__cond);

extern int pthread_cond_wait (pthread_cond_t *__restrict __cond, pthread_mutex_t *__restrict __mutex);
extern int pthread_cond_timedwait (pthread_cond_t *__restrict __cond, pthread_mutex_t *__restrict __mutex, const struct timespec *__restrict __abstime);
extern int pthread_condattr_init (pthread_condattr_t *__attr);
extern int pthread_condattr_destroy (pthread_condattr_t *__attr);

extern int pthread_mutexattr_init(pthread_mutexattr_t *attr);


#ifdef __cplusplus
}
#endif
#endif //POSIX_PTHREAD_H
