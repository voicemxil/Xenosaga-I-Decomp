/* PS2 SDK sceGs trivial accessors.
 *
 * Returns the address of an internal static table; the modern
 * assembler's constant synthesis for the raw address picks lui+ori
 * rather than the original's lui+addiu (same issue as sceSif.c), so
 * this is written as inline asm reproducing the exact instructions.
 */

void *sceGsGetGParam(void)
{
    __asm__ __volatile__(
        "lui $2,0x4b\n\t"
        "addiu $2,$2,-18112"
    );
}
