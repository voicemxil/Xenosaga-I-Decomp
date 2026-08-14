/*
 * libgcc2.c L__main member (full vendored libgcc2.c in newlib/gccsrc).
 * The target's DO_GLOBAL_DTORS_BODY / DO_GLOBAL_CTORS_BODY macros add
 * DWARF frame-info (de)registration around the standard ctor/dtor
 * walks, plus an atexit(__do_global_dtors) hookup at the end of
 * __do_global_ctors (in place of ON_EXIT, which stays empty so no
 * _exit_dummy_ref global is emitted).
 */

struct object
{
  void *pc_begin;
  void *pc_end;
  void *fde_begin;
  void *fde_array;
  unsigned long count;
  struct object *next;
};

extern char __EH_FRAME_BEGIN__[];
extern void __register_frame_info (void *, struct object *);
extern void *__deregister_frame_info (void *);
extern int atexit (void (*) (void));

#define DO_GLOBAL_DTORS_BODY						\
{									\
  static func_ptr *p = __DTOR_LIST__ + 1;				\
  static int completed = 0;						\
  while (*p)								\
    {									\
      func_ptr f = *p;							\
      p++;								\
      f ();								\
    }									\
  if (completed)							\
    return;								\
  completed = 1;							\
  __deregister_frame_info (__EH_FRAME_BEGIN__);				\
}

#define DO_GLOBAL_CTORS_BODY						\
{									\
  static struct object object;						\
  unsigned long long nptrs;							\
  unsigned i;								\
  __register_frame_info (__EH_FRAME_BEGIN__, &object);			\
  nptrs = (unsigned long long) __CTOR_LIST__[0];				\
  if (nptrs == (unsigned long long)-1)					\
    for (nptrs = 0; __CTOR_LIST__[nptrs + 1] != 0; nptrs++);		\
  for (i = nptrs; i >= 1; i--)						\
    __CTOR_LIST__[i] ();						\
  atexit (__do_global_dtors);						\
}

#define L__main
#include "libgcc2.c"
