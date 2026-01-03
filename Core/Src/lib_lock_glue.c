#if !defined (__GNUC__) || defined (__CC_ARM)
#error "lib_lock_glue.c" should be used with GNU Compilers only
#endif /* !defined (__GNUC__) || defined (__CC_ARM) */

/* Includes ------------------------------------------------------------------*/
#include <cmsis_compiler.h>

/**
  * @brief Global Error_Handler
  */
__WEAK void Error_Handler(void)
{
  /* Not used if it exists in project */
  while (1);
}

#ifdef __SINGLE_THREAD__
#warning C library is in single-threaded mode. Please take care when using C library functions in threaded contexts
#else

#include <newlib.h>
#include <stdatomic.h>
#include "stm32_lock.h"

#if __NEWLIB__ >= 3 && defined (_RETARGETABLE_LOCKING)
#include <errno.h>
#include <stdlib.h>
#include <sys/lock.h>

#define STM32_LOCK_PARAMETER(lock) (&(lock)->lock_data)

struct __lock
{
  LockingData_t lock_data;
};

struct __lock __lock___sinit_recursive_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___sfp_recursive_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___atexit_recursive_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___at_quick_exit_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___malloc_recursive_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___env_recursive_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___tz_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___dd_hash_mutex = { LOCKING_DATA_INIT };
struct __lock __lock___arc4random_mutex = { LOCKING_DATA_INIT };

void __retarget_lock_init(_LOCK_T *lock)
{
  __retarget_lock_init_recursive(lock);
}

void __retarget_lock_init_recursive(_LOCK_T *lock)
{
  if (lock == NULL)
  {
    errno = EINVAL;
    return;
  }

  *lock = (_LOCK_T)malloc(sizeof(struct __lock));
  if (*lock != NULL)
  {
    stm32_lock_init(STM32_LOCK_PARAMETER(*lock));
    return;
  }

  STM32_LOCK_BLOCK();
}

void __retarget_lock_close(_LOCK_T lock)
{
  __retarget_lock_close_recursive(lock);
}

void __retarget_lock_close_recursive(_LOCK_T lock)
{
  free(lock);
}

void __retarget_lock_acquire(_LOCK_T lock)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(lock);
  stm32_lock_acquire(STM32_LOCK_PARAMETER(lock));
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(lock);
  stm32_lock_acquire(STM32_LOCK_PARAMETER(lock));
}

int __retarget_lock_try_acquire(_LOCK_T lock)
{
  __retarget_lock_acquire(lock);
  return 0;
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
  __retarget_lock_acquire_recursive(lock);
  return 0;
}

void __retarget_lock_release(_LOCK_T lock)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(lock);
  stm32_lock_release(STM32_LOCK_PARAMETER(lock));
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(lock);
  stm32_lock_release(STM32_LOCK_PARAMETER(lock));
}

#else
#warning This makes malloc, env, and TZ calls thread-safe, not the entire newlib

#include <reent.h>

static LockingData_t __lock___malloc_recursive_mutex = LOCKING_DATA_INIT;
static LockingData_t __lock___env_recursive_mutex = LOCKING_DATA_INIT;
static LockingData_t __lock___tz_mutex = LOCKING_DATA_INIT;

#if __STD_C
void __malloc_lock(struct _reent *reent)
{
  STM32_LOCK_UNUSED(reent);
  stm32_lock_acquire(&__lock___malloc_recursive_mutex);
}

void __malloc_unlock(struct _reent *reent)
{
  STM32_LOCK_UNUSED(reent);
  stm32_lock_release(&__lock___malloc_recursive_mutex);
}
#else
void __malloc_lock()
{
  stm32_lock_acquire(&__lock___malloc_recursive_mutex);
}

void __malloc_unlock()
{
  stm32_lock_release(&__lock___malloc_recursive_mutex);
}
#endif /* __STD_C */

void __env_lock(struct _reent *reent)
{
  STM32_LOCK_UNUSED(reent);
  stm32_lock_acquire(&__lock___env_recursive_mutex);
}

void __env_unlock(struct _reent *reent)
{
  STM32_LOCK_UNUSED(reent);
  stm32_lock_release(&__lock___env_recursive_mutex);
}

void __tz_lock()
{
  stm32_lock_acquire(&__lock___tz_mutex);
}

void __tz_unlock()
{
  stm32_lock_release(&__lock___tz_mutex);
}

#endif /* __NEWLIB__ >= 3 && defined (_RETARGETABLE_LOCKING) */

typedef struct
{
  atomic_uchar initialized;
  uint8_t acquired;
  uint16_t unused;
} __attribute__((packed)) CxaGuardObject_t;

static LockingData_t __cxa_guard_mutex = LOCKING_DATA_INIT;

int __cxa_guard_acquire(CxaGuardObject_t *guard_object)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(guard_object);

  if (atomic_load(&guard_object->initialized) == 0)
  {
    stm32_lock_acquire(&__cxa_guard_mutex);
    if (atomic_load(&guard_object->initialized) == 0)
    {
      if (guard_object->acquired)
      {
        STM32_LOCK_BLOCK();
      }

      guard_object->acquired = 1;
      return 1;
    }
    else
    {
      stm32_lock_release(&__cxa_guard_mutex);
    }
  }

  return 0;
}

void __cxa_guard_abort(CxaGuardObject_t *guard_object)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(guard_object);

  if (guard_object->acquired)
  {
    guard_object->acquired = 0;
    stm32_lock_release(&__cxa_guard_mutex);
  }
  else
  {
    STM32_LOCK_BLOCK();
  }
}

void __cxa_guard_release(CxaGuardObject_t *guard_object)
{
  STM32_LOCK_BLOCK_IF_NULL_ARGUMENT(guard_object);

  atomic_store(&guard_object->initialized, 1);

  __cxa_guard_abort(guard_object);
}

#endif /* __SINGLE_THREAD__ */
