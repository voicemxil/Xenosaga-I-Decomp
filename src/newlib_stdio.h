/*
 * Shared newlib 1.8.2 stdio definitions for the newlib_* stdio TUs.
 * The __sFILE layout is stock 1.8.2, confirmed against the disassembly
 * (_flags@12, _file@14, _offset@80, _data@84, sizeof(FILE)=88 from
 * _fwalk's stride; __sglue@472 in struct _reent from _fwalk's
 * `addiu s2,a0,472`). Only fields these TUs touch are modelled.
 */

typedef void *_PTR;
typedef long off_t;
typedef long _fpos_t;
typedef _fpos_t fpos_t;

#define SEEK_END 2

#define __SOFF 0x1000 /* set iff _offset is in fact correct */
#define __SAPP 0x0100 /* fdopen()ed in append mode - so must write to end */

struct _reent; /* opaque here: only passed through to the _r syscalls */

struct __sbuf
{
  unsigned char *_base;
  int _size;
};

typedef struct __sFILE
{
  unsigned char *_p;   /*  0: current position in (some) buffer */
  int _r;              /*  4: read space left for getc() */
  int _w;              /*  8: write space left for putc() */
  short _flags;        /* 12: flags, below; this FILE is free if 0 */
  short _file;         /* 14: fileno, if Unix descriptor, else -1 */
  struct __sbuf _bf;   /* 16: the buffer (at least 1 byte, if !NULL) */
  int _lbfsize;        /* 24: 0 or -_bf._size, for inline putc */
  _PTR _cookie;        /* 28: cookie passed to io functions */
  int (*_read) ();     /* 32 */
  int (*_write) ();    /* 36 */
  _fpos_t (*_seek) (); /* 40 */
  int (*_close) ();    /* 44 */
  struct __sbuf _ub;   /* 48: ungetc buffer */
  unsigned char *_up;  /* 56: saved _p when _p is doing ungetc data */
  int _ur;             /* 60: saved _r when _r is counting ungetc data */
  unsigned char _ubuf[3]; /* 64: guarantee an ungetc() buffer */
  unsigned char _nbuf[1]; /* 67: guarantee a getc() buffer */
  struct __sbuf _lb;   /* 68: buffer for fgetline() */
  int _blksize;        /* 76: stat.st_blksize (may be != _bf._size) */
  int _offset;         /* 80: current lseek offset */
  struct _reent *_data; /* 84: points to reentrancy data */
} FILE;

/* Declared exactly as newlib 1.8.2 reent.h does: _read_r/_write_r
   return _ssize_t (= long, 64-bit here), which is what forces the
   original's dsll32/dsra32 truncation of their results back to int
   and stops __swrite from tail-calling _write_r. */
typedef unsigned int size_t;
typedef long _ssize_t;
extern _ssize_t _read_r (struct _reent *, int, void *, size_t);
extern _ssize_t _write_r (struct _reent *, int, const void *, size_t);
extern off_t _lseek_r (struct _reent *, int, off_t, int);
extern int _close_r (struct _reent *, int);

