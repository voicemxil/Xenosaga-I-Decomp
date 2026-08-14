/* Minimal stand-in for gcc mainline tsystem.h: the freestanding
   system-header shim. frame.c needs malloc/free/size_t and NULL. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
