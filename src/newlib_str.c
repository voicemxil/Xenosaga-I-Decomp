/*
 * newlib (~1.8.x vintage) string.c pure-ANSI functions that are NOT
 * PS2 MMI SIMD (unlike memcpy/memset/strcpy/strlen/strncpy/strncmp/
 * strchr/memmove/memchr/memcmp, which are all PS2-specific 128-bit
 * lq/sq/pcpyld/pnor/pand hardware routines and belong in HARDWARE).
 * strstr and strrchr disassemble as plain scalar byte loops -- real
 * newlib source, transcribed verbatim.
 */

typedef unsigned int size_t;

/* newlib/libc/string/strstr.c */
char *
strstr (const char *searchee, const char *lookfor)
{
  if (*searchee == 0)
    {
      if (*lookfor)
        return (char *) 0;
      return (char *) searchee;
    }

  while (*searchee)
    {
      size_t i;
      i = 0;

      while (1)
        {
          if (lookfor[i] == 0)
            {
              return (char *) searchee;
            }

          if (lookfor[i] != searchee[i])
            {
              break;
            }
          i++;
        }
      searchee++;
    }

  return (char *) 0;
}

/* newlib/libc/string/strrchr.c */
char *
strrchr (const char *s, int i)
{
  const char *last = (const char *) 0;
  char c = i;

  while (*s)
    {
      if (*s == c)
        {
          last = s;
        }
      s++;
    }

  if (*s == c)
    {
      last = s;
    }

  return (char *) last;
}
