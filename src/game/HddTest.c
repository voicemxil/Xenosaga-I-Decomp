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

/* TODO: near-miss, 12 of 47 words. The instruction MULTISET is the
 * original's -- this is pure intra-block scheduling, not a shape or
 * allocation difference. The whole function up to the loop is one basic
 * block (nothing branches), and the original issues the six callee-saved
 * stores contiguously with `i = 0` in sceOpen's delay slot and the two
 * `lui` %hi bases for the loop's string constants AFTER the first printf;
 * gcc interleaves `move s2,zero` and one `lui` between the stores and
 * puts the other `lui` in sceOpen's delay slot.
 * Ruled out this session with numbers (all on HddTestHddFull):
 *   -O2 -G8                        12 diffs   (current)
 *   -fno-schedule-insns            14 diffs
 *   -fno-schedule-insns2           18 diffs
 *   -fno-gcse / -fno-strength-reduce / -fno-expensive-optimizations /
 *   -fno-caller-saves / -fno-peephole   all 12 diffs (no effect)
 * So a per-file cflag will not close it; it needs either a source shape
 * that changes the block's scheduling priorities or ~6 reordering-flag
 * sites, which is more than this is worth. */
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

/* Fill the common partition with 256 numbered directories.

   The three digits must be produced by REPEATEDLY dividing a running
   value by 10, not by i%10 / i/10%10 / i/100%10: the latter lets gcc
   compute i/100 directly (an extra div/div1 pair). The loop also has to
   exit through a `break` on the sceMkdir failure rather than a compound
   `while (r >= 0 && i < 256)` -- the compound form rotates the loop, and
   a rotated loop has loop-invariant motion off, so the shared `li 10`
   divisor is rematerialised three times inside instead of once above. */
void HddTestDummyFolder(void)
{
    static char name[] = "pfs1:/000";
    int i;
    int n;
    int r;

    HddTestMountCommon();
    for (i = 0; i < 256; i++) {
        n = i;
        name[8] = n % 10 + '0';
        n = n / 10;
        name[7] = n % 10 + '0';
        n = n / 10;
        name[6] = n % 10 + '0';
        r = sceMkdir(name, 511);
        printf("%s:%d\n", name, r);
        if (r < 0) {
            break;
        }
    }
    HddTestUnmountCommon();
}

int sceChstat(const char *path, void *stat, int mask);

/* NEAR MISS -- 2 diffs, REGISTER ($v0 -> $v1), both in the loop
   preheader: the original materialises the static's address as
   `lui $v0,%hi(name)` + `addiu $s2,$v0,%lo(name)` and we get $v1 for
   that temp. Everything else, all 86 words, is identical. Swept: every
   ordering of the four locals; st as unsigned int[16] / char[64] / via a
   pointer; the 0xC4A7 mode as a literal, a named local, and a cast
   store; the path string through a `char *`. The temp is the LAST of the
   four loop invariants gcc hoists (after $s4, $s5, $s6) and its register
   is an allocator naming tie-break, not source-reachable; a whole-
   function --swap-regs is not usable because $v0 and $v1 are both live
   elsewhere in the body and already agree there.

   Create 1024 save folders under Your Saves, chmod'ing each one, and stop
   at the first failure */
void HddTest1024Save(void)
{
    static char name[] = "pfs1:/Your Saves/0000";
    unsigned int st[16];
    int i;
    int n;
    int r;

    HddTestMountCommon();
    HddTestMakeYourSaves();
    for (i = 0; i < 1024; i++) {
        n = i;
        name[20] = n % 10 + '0';
        n = n / 10;
        name[19] = n % 10 + '0';
        n = n / 10;
        name[18] = n % 10 + '0';
        n = n / 10;
        name[17] = n % 10 + '0';
        r = sceMkdir(name, 511);
        if ((i & 0x1F) == 0) {
            printf("%s:%d\n", name, r);
        }
        if (r < 0) {
            printf("%s:%d\n", name, r);
            break;
        }
        st[1] = 0xC4A7;
        r = sceChstat(name, st, 2);
        if (r < 0) {
            printf("chstat:%d\n", r);
            break;
        }
    }
    HddTestUnmountCommon();
}
