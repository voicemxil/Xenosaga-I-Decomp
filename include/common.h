/*
 * Shared primitive typedefs for game-code translation units.
 * Adoption is incremental: a TU that includes this must drop its own
 * copies of these typedefs. Byte matching is unaffected -- these are
 * exactly the definitions the TUs already repeat locally.
 */

#ifndef COMMON_H
#define COMMON_H

typedef signed char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef float f32;

#ifndef NULL
#define NULL 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define ARRAY_COUNT(arr) (int)(sizeof(arr) / sizeof((arr)[0]))

#endif /* COMMON_H */
