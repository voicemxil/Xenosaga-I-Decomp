/* PS2 SDK filesystem driver dispatch wrappers */

extern int _sceCallCode(void *arg0, int code);

int sceRemove(void *path)
{
    return _sceCallCode(path, 6);
}

int sceRmdir(void *path)
{
    return _sceCallCode(path, 8);
}

int sceDelDrv(void *path)
{
    return _sceCallCode(path, 16);
}

int sceChdir(void *path)
{
    return _sceCallCode(path, 18);
}

int sceUmount(void *path)
{
    return _sceCallCode(path, 21);
}

/* sceFsReset: clears the fs-init flag and the 4-byte fs-version buffer,
 * then returns 0. Both are named fixed addresses from
 * config/symbol_addrs.txt (_fs_init, _fsversion), so referencing them
 * by name lets the linker's ordinary %hi/%lo relocation produce
 * lui+addiu, same as the original -- real C, no inline asm needed. */

extern void *memset(void *s, int c, unsigned int n);
extern int _fs_init;
extern char _fsversion[4];

int sceFsReset(void)
{
    _fs_init = 0;
    memset(_fsversion, 0, 4);
    return 0;
}
