/* PS2 SDK scePrintf: printf() routed through the DECI2 console.
 *
 * The libc formatter calls out through the `_putchar` hook; scePrintf
 * swaps in deci2Putchar for the duration of one call and restores
 * whatever was there before, so it works no matter what the rest of the
 * program has the hook pointed at.
 *
 * <stdarg.h> is the compiler's own header. It matters which one: the
 * EABI va_start biases the pointer back over the unused integer
 * argument registers, which is exactly the a1..t3 spill block and the
 * `addiu a1,sp,120` the original emits. The __builtin_stdarg_start
 * spelling other files in this tree use is a gcc 2.96 builtin and does
 * not exist in the 2.9 SDK compiler.
 */

#include <stdarg.h>

extern int (*_putchar)(int c);
extern int deci2Putchar(int c);
extern int _printf(const char *fmt, va_list ap);

int scePrintf(const char *fmt, ...)
{
    va_list ap;
    int (*old)(int);
    int r;

    old = _putchar;
    _putchar = deci2Putchar;
    va_start(ap, fmt);
    r = _printf(fmt, ap);
    va_end(ap);
    _putchar = old;
    return r;
}
