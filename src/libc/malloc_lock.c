/*
 * Sony's __malloc_lock/__malloc_unlock: a recursive mutex over the SDK
 * semaphore in __sce_sema_id, keyed by GetThreadId(). Stock newlib
 * mlock.c is a pair of empty stubs; this implementation is Sony glue,
 * reconstructed from the disassembly (owner id at __malloc_lock_owner,
 * recursion depth in call_count).
 */

struct _reent;

extern int GetThreadId (void);
extern int WaitSema (int);
extern int SignalSema (int);

extern int __sce_sema_id[];
#define __sce_sema_id (__sce_sema_id[0])
extern int __malloc_lock_owner[];
#define __malloc_lock_owner (__malloc_lock_owner[0])
extern int call_count[];
#define call_count (call_count[0])

void
__malloc_lock (ptr)
     struct _reent *ptr;
{
  int tid;

  tid = GetThreadId ();
  if (__malloc_lock_owner == tid)
    {
      call_count = call_count + 1;
      return;
    }
  WaitSema (__sce_sema_id);
  __malloc_lock_owner = tid;
  call_count = call_count + 1;
}

void
__malloc_unlock (ptr)
     struct _reent *ptr;
{
  call_count = call_count - 1;
  if (call_count == 0)
    {
      __malloc_lock_owner = -1;
      SignalSema (__sce_sema_id);
    }
}
