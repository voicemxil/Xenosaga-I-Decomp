/* Internal PS2 HDD unit test harness (debug menu) */

int sceMount(const char *pfs, const char *dev, int flag, void *arg, int len);
int sceUmount(const char *pfs);
int sceFormat(const char *dev, const char *fs, void *arg, int len);
int sceMkdir(const char *path, int mode);
int sceDevctl(const char *dev, int cmd, void *arg, int alen, void *buf, int blen);
int sceOpen(const char *path, int flag);
int sceIoctl2(int fd, int cmd, void *arg, int alen, void *buf, int blen);
int sceClose(int fd);
int printf(const char *fmt, ...);

/* TODO: near-miss (38/42 words present but LENGTH-short by 4; original
 * spills the second sceDevctl result into a genuinely callee-saved s1
 * register that our compiler never needs since nothing crosses a call --
 * looks like a pure register-allocator choice, not source-reachable).
 * Parked per budget rule. */
/* Create the Your Saves folder on the mounted common partition and print
 * its zone size/free stats */
int HddTestMakeYourSaves(void)
{
    int r;
    int n;
    const char *pfs1 = "pfs1:";

    r = sceMkdir("pfs1:/Your Saves", 511);
    printf("make YourSaves:%d\n", r);
    sceDevctl(pfs1, 20481, 0, 0, 0, 0);
    n = sceDevctl(pfs1, 20482, 0, 0, 0, 0);
    n = printf("zonesz:%d\n", n);
    return printf("zonefree:%d\n", n);
}

/* Mount the common PFS partition, printing the result */
void HddTestMountCommon(void)
{
    int r = sceMount("pfs1:", "hdd0:__common", 4, 0, 0);
    printf("mount:%d\n", r);
}

/* Unmount the common PFS partition, printing the result */
void HddTestUnmountCommon(void)
{
    int r = sceUmount("pfs1:");
    printf("umount:%d\n", r);
}

/* Mount, populate, and unmount the Your Saves folder */
void HddTestMakeYS(void)
{
    HddTestMountCommon();
    HddTestMakeYourSaves();
    HddTestUnmountCommon();
}

/* Power down the HDD unit, ignoring errors */
void HddTestShutdown(void)
{
    printf("shutdown hdd\n");
    sceDevctl("pfs:", 20483, 0, 0, 0, 0);
    sceDevctl("hdd:", 18438, 0, 0, 0, 0);
}

/* Low-level format the whole drive, then re-format the common partition */
void HddTestFormat(void)
{
    int r;
    int nSize;

    printf("format HDD\n");
    r = sceFormat("hdd0:", 0, 0, 0);
    printf(" result:%d\n", r);
    nSize = 8192;
    r = sceFormat("pfs:", "hdd0:__common", &nSize, 4);
    printf(" format __common:%d\n", r);
}

/* TODO: near-miss (35/47 words match; original callee-saved prologue
 * save order is s1,s2,s0,s3,s4,ra -- an unusual first-use order our
 * compiler doesn't reproduce via any local declaration order tried.
 * Parked per budget rule after 1 attempt. */
/* Repeatedly write sub-partitions until the drive fills up, logging each
 * result */
void HddTestHddFull(void)
{
    int fd;
    int i;
    int r;

    fd = sceOpen("hdd0:PP.SLPS-99999.DUMMY.DUMMY,,,1G,PFS", 515);
    printf("create partition:%d\n", fd);
    i = 0;
    do {
        r = sceIoctl2(fd, 26625, "1G", 3, 0, 0);
        printf("sub%d:%d\n", i, r);
        i++;
    } while (r >= 0);
    r = sceClose(fd);
    printf("close:%d\n", r);
}
