#include "pthread-test.h"

void _add_wait_list(Waiter **wait_list, Waiter *waiter)
{
  if (*wait_list == NULL) {
    // Nothing in the list so add it directly.
    // Update prev and next pointer to reference self
    *wait_list = waiter;
    waiter->next = waiter;
    waiter->prev = waiter;
  } else {
    // Add after the last element
    Waiter *first = *wait_list;
    Waiter *last = (*wait_list)->prev;

    // Update new entry
    waiter->next = first;
    waiter->prev = last;

    // Insert into the list
    first->prev = waiter;
    last->next = waiter;
  }
  waiter->in_list = true;
}

void _remove_wait_list(Waiter **wait_list, Waiter *waiter)
{
  Waiter *prev = waiter->prev;
  Waiter *next = waiter->next;

  // Remove from list
  prev->next = waiter->next;
  next->prev = waiter->prev;
  *wait_list = waiter->next;

  if (*wait_list == waiter) {
    // This was the last element in the list
    *wait_list = NULL;
  }

  // Invalidate pointers
  waiter->next = NULL;
  waiter->prev = NULL;
  waiter->in_list = false;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
  memset (attr, 0, sizeof (*attr));
  attr->name = "application_unnamed_mutex";
  attr->attr_bits = osMutexPrioInherit;
  return 0;
}

extern int pthread_mutex_init (pthread_mutex_t *__mutex, pthread_mutexattr_t *__mutexattr)
{
  __mutex = calloc (1, sizeof (*__mutex));
  memset(&__mutex->_obj_mem, 0, sizeof(__mutex->_obj_mem));
  __mutexattr->cb_mem = &__mutex->_obj_mem;
  __mutexattr->cb_size = sizeof(__mutex->_obj_mem);

  __mutex->_id = osMutexNew(__mutexattr);
  return __mutex->_id;
}

extern int pthread_mutex_destroy (pthread_mutex_t *__mutex)
{
  return 0;
}

static int pthread_mutex_lock_for (pthread_mutex_t * __mutex, int millisec)
{
  osMutexAcquire (__mutex->_id, millisec);
  return 0;
}


extern int pthread_mutex_lock (pthread_mutex_t * __mutex)
{
  osMutexAcquire(__mutex->_id, osWaitForever);
  return 0;
}

extern int pthread_mutex_unlock (pthread_mutex_t *__mutex)
{
  return osMutexRelease(__mutex->_id);
}


extern int pthread_cond_init (pthread_cond_t *__restrict __cond, pthread_condattr_t *__restrict __cond_attr)
{
  memset (&__cond->_obj_mem, 0, sizeof (__cond->_obj_mem));
  if (__cond_attr == NULL)
  {
    pthread_condattr_t __cond_attr_new;
    memset (&__cond_attr_new, 0, sizeof (__cond_attr_new));

    __cond_attr_new.cb_mem = &__cond->_obj_mem;
    __cond_attr_new.cb_size = sizeof (__cond->_obj_mem);
    __cond->_id = osSemaphoreNew (10, 0xffff, &__cond_attr_new);
  }
  else
  {
    __cond_attr->cb_mem = &__cond->_obj_mem;
    __cond_attr->cb_size = sizeof (__cond->_obj_mem);
    __cond->_id = osSemaphoreNew (10, 0xffff, __cond_attr);
  }

  return 0;
}

extern int pthread_cond_destroy (pthread_cond_t *__cond)
{
  return osSemaphoreRelease(__cond->_id);
}

extern int pthread_cond_signal (pthread_cond_t *__cond)
{
  int ret = -1;

  if (__cond->waiter._wait_list != NULL) {
    pthread_cond_destroy(__cond->waiter._wait_list->sem);
    _remove_wait_list(&__cond->waiter._wait_list, __cond->waiter._wait_list);

    ret = 0;
  }
  return ret;
}

extern int pthread_cond_broadcast (pthread_cond_t *__cond)
{
  while (__cond->waiter._wait_list != NULL) {
    pthread_cond_destroy(__cond->waiter._wait_list->sem);
    _remove_wait_list(&__cond->waiter._wait_list, __cond->waiter._wait_list);
  }

  return 0;
}


static int32_t wait (uint32_t millisec, pthread_cond_t *__restrict __cond)
{
  osStatus_t stat = osSemaphoreAcquire(__cond->_id, millisec);
  switch (stat){
    case osOK:
      return osSemaphoreGetCount(__cond->_id) + 1;
    case osErrorTimeout:
    case osErrorResource:
      return 0;
    case osErrorParameter:
    default:
      return -1;
  }
}

extern int pthread_cond_wait (pthread_cond_t *__restrict __cond, pthread_mutex_t *__restrict __mutex)
{
  Waiter current_thread;
  _add_wait_list(&__cond->waiter._wait_list, &current_thread);

  pthread_mutex_unlock (__mutex);

  int32_t sem_count = wait (osWaitForever ,current_thread.sem);
  bool timeout = (sem_count > 0) ? false : true;

  pthread_mutex_lock (__mutex);

  if (current_thread.in_list) {
    _remove_wait_list(&__cond->waiter._wait_list, &current_thread);
  }

  return timeout;
}

static int convert_abstime_to_mili (const struct timespec *__restrict __abstime)
{
  int mili;
  if (__abstime->tv_nsec)
  {
    mili = __abstime->tv_nsec / 1000000;
  }

  if (__abstime->tv_sec)
  {
    mili += 1000 * __abstime->tv_sec;
  }

  return mili;
}

extern int pthread_cond_timedwait (pthread_cond_t *__restrict __cond, pthread_mutex_t *__restrict __mutex, const struct timespec *__restrict __abstime)
{
  Waiter current_thread;
  _add_wait_list(&__cond->waiter._wait_list, &current_thread);

  pthread_mutex_unlock (__mutex);
  int32_t sem_count = wait (convert_abstime_to_mili (__abstime), current_thread.sem);
  bool timeout = (sem_count > 0) ? false : true;

  pthread_mutex_lock (__mutex);

  if (current_thread.in_list) {
    _remove_wait_list(&__cond->waiter._wait_list, &current_thread);
  }

  return timeout;
}

extern int pthread_condattr_init (pthread_condattr_t *__attr)
{
  __attr->name = "App";
  return 0;
}

extern int pthread_condattr_destroy (pthread_condattr_t *__attr)
{
  __attr = NULL;
  return 0;
}

extern int pthread_mutexattr_setprotocol (pthread_mutexattr_t *__attr, int __protocol)
{
  int ret = -1;
  if (__protocol == osMutexRecursive || __protocol == osMutexPrioInherit || __protocol == osMutexRobust)
  {
    __attr->attr_bits = __protocol;
    ret = 0;
  }

  return ret;
}