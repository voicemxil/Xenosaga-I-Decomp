/* Sony's gthr configuration for the PS2 EE, reconstructed from the
   disassembly: every frame-registry mutex operation is a WaitSema/
   SignalSema pair on the semaphore id in __sce_eh_sema_id (the EH
   sibling of malloc's __sce_sema_id -- they sit next to each other in
   .bss). The mutex object itself is unused; the array-decl trick
   keeps the id out of small data, matching the original's absolute
   lui/lw addressing. */

#ifndef __gthr_ps2_h
#define __gthr_ps2_h

#define __GTHREADS 1

typedef int __gthread_key_t;
typedef int __gthread_once_t;
typedef int __gthread_mutex_t;

#define __GTHREAD_ONCE_INIT 0
#define __GTHREAD_MUTEX_INIT 0

extern int WaitSema (int);
extern int SignalSema (int);

extern int __sce_eh_sema_id[];

static inline int
__gthread_active_p (void)
{
  return 1;
}

static inline int
__gthread_once (__gthread_once_t *once, void (*func) (void))
{
  return 0;
}

static inline int
__gthread_mutex_lock (__gthread_mutex_t *mutex)
{
  return WaitSema (__sce_eh_sema_id[0]);
}

static inline int
__gthread_mutex_trylock (__gthread_mutex_t *mutex)
{
  return 0;
}

static inline void
__gthread_mutex_unlock (__gthread_mutex_t *mutex)
{
  SignalSema (__sce_eh_sema_id[0]);
  __asm__ __volatile__ ("");
}

#endif /* __gthr_ps2_h */
