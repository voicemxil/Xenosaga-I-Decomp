/* PS2 SDK sceMc (memory card) thin wrappers. */

/* sceMcMkdir: opens the directory with sceMcOpen (mode 64, i.e.
 * O_CREAT-ish) and, if it returns a falsy fd, stores a fixed error code
 * into `mcRunCmdNo`, a named fixed address from
 * config/symbol_addrs.txt -- so this compiles as real C, no inline asm
 * needed for the raw address. */

extern int sceMcOpen(int a0, int a1, int a2, int a3);
extern int mcRunCmdNo;

int sceMcMkdir(int a0, int a1, int a2)
{
    int fd = sceMcOpen(a0, a1, a2, 64);

    if (!fd)
        mcRunCmdNo = 11;
    return fd;
}
