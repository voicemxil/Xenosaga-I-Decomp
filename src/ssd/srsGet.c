/* srs (stage-resource-script) effect/combo table accessors */

extern char _loadEsdData[];
int srsGetEffect2Idx(int id);

/* Resolve an effect id to its raw esd-data blob (16-byte stride table) */
void *srsGetEsdData(int id)
{
    int idx;

    if ((unsigned int)id >= 0xC00) {
        return 0;
    }
    idx = id - srsGetEffect2Idx(id);
    return _loadEsdData + idx * 16;
}

/* Same as srsGetEsdData but the caller has already resolved the index */
void *srsGetEsdData2(int idx)
{
    if ((unsigned int)idx >= 0xC00) {
        return 0;
    }
    return _loadEsdData + idx * 16;
}

extern int srsLoadMode;

int srsGetLoadMode(void)
{
    return srsLoadMode;
}

/* srsGetEffectData is just an alias for srsGetEsdData */
void *srsGetEffectData(int id)
{
    return srsGetEsdData(id);
}

extern int _loadComboData[];

/* Look up a combo-data entry, or 0 if out of range / unset */
/* Look up a combo-data entry, or 0 if out of range / unset.
 * Guard-clause shape (not a single `ret` accumulator): with the
 * accumulator gcc parks the result in $a1 and adds a tail `move v0,a1`;
 * the early returns let it build the movn straight into $v0. */
void *srsGetComboData(int idx)
{
    int *p;

    if (idx >= 0xC3) {
        return 0;
    }
    p = &_loadComboData[idx];
    if (*p == 0) {
        return 0;
    }
    return p;
}

int sprintf(char *buf, const char *fmt, ...);
extern char msg_0_00795120[];
extern char D_004CC6A8[];
extern char D_004DBB20[];

/* Format an effect's display name, or 0 if the effect has no name */
char *srsGetEffectName(int id)
{
    void *data;

    data = srsGetEffectData(id);
    if (data == 0) {
        return data;
    }
    if (*(signed char *)data == 0) {
        return 0;
    }
    sprintf(msg_0_00795120, D_004CC6A8, data, D_004DBB20);
    return msg_0_00795120;
}

extern char msg_1_007951A0[];
extern char D_004CC6B8[];
extern char D_004DBB28[];

/* Alternate effect-name formatter (no empty-name guard) */
char *srsGetEffectName2(int id)
{
    void *data;

    data = srsGetEffectData(id);
    if (data == 0) {
        return data;
    }
    sprintf(msg_1_007951A0, D_004CC6B8, data, D_004DBB28);
    return msg_1_007951A0;
}

extern char msg_2_00795220[];

/* Format a combo's display name, or 0 if the combo has no name */
char *srsGetComboName(int idx)
{
    int *data;

    data = srsGetComboData(idx);
    if (data == 0) {
        return (char *)data;
    }
    if (*data == 0) {
        return 0;
    }
    sprintf(msg_2_00795220, D_004CC6B8, *data, D_004DBB28);
    return msg_2_00795220;
}

/* --- srs CD-read state accessors --- */

extern int _nRead;

void srsInitCdRead(void)
{
    _nRead = 0;
}

int srsLeaveCdRead(void)
{
    return _nRead;
}

void srsSetLoadMode(int nMode)
{
    srsLoadMode = nMode;
}

extern char srsViewPath[];
char *strcpy(char *, const char *);

void srsSetViewPath(char *pPath)
{
    strcpy(srsViewPath, pPath);
}

extern int fileLoad(int a, int b, int c);

int srsFileLoad(int a, int b, int c)
{
    return fileLoad(a, b, c);
}

/* --- weapon <-> effect id cross-reference table (45 x 8 bytes) --- */

typedef struct
{
    short nWeaponID;   /* 0x00 */
    short nEffectID;   /* 0x02 */
    short nAltID1;     /* 0x04 */
    short nAltID2;     /* 0x06 */
} WEAPON_ENT;

extern WEAPON_ENT _weaponTbl[];

int srsGetWeaponEffectIdx(int nEftNo);

/* Effect number -> weapon id (0 when the effect has no weapon slot) */
int srsEftNoWeaponEffectID(int nEftNo)
{
    int nIdx;
    int nRet;

    nIdx = srsGetWeaponEffectIdx(nEftNo);
    nRet = 0;
    if (nIdx >= 0) {
        nRet = _weaponTbl[nIdx].nEffectID;
    }
    return nRet;
}

int srsEftNo2WeaponID(int nEftNo)
{
    int nIdx;
    int nRet;

    nIdx = srsGetWeaponEffectIdx(nEftNo);
    nRet = 0;
    if (nIdx >= 0) {
        nRet = _weaponTbl[nIdx].nWeaponID;
    }
    return nRet;
}

/* Linear search the other way: weapon id -> effect id, -1 if absent */
int srsWeapon2EffectID(int nWeaponID)
{
    int i;

    if (nWeaponID > 0) {
        for (i = 0; i < 45; i++) {
            if (nWeaponID == _weaponTbl[i].nWeaponID) {
                return _weaponTbl[i].nEffectID;
            }
        }
    }
    return -1;
}

/* Load an effect's data file by id; -1 if the id has no name entry */
extern char *srsGetEffectName(int nID);
int srsLoadEffectData(void *pBuf, int nID)
{
    char *pName;

    pName = srsGetEffectName(nID);
    if (pName == 0) {
        return -1;
    }
    return fileLoad((int)pBuf, (int)pName, 1);
}

/* Rewrite unix path separators in place to the CD filesystem's.
 * Index the array rather than walking a `char *`: with a pointer
 * variable gcc CSEs the loop-test read into the '/' test and pays an
 * lbu + sll/sra sign-extension for it; the subscript form keeps the two
 * separate `lb`s the original has. `i = 0` must sit BETWEEN the two
 * guards -- that is the instruction gcc puts in the second beqz's delay
 * slot. */
void srsChangeSeparator(char *pPath)
{
    int i;

    if (pPath != 0) {
        i = 0;
        while (pPath[i] != 0) {
            if (pPath[i] == '/') {
                pPath[i] = '\\';
            }
            i++;
        }
    }
}

/* --- boss "wait" effect lookup (36 x 6-byte records) --- */

typedef struct
{
    short nID;      /* 0x00 */
    short nEftNo;   /* 0x02 */
    short f04;      /* 0x04 */
} CHR_EFT;

extern CHR_EFT _srsChrEfTbl3[];

int srsGetBossWaitEftNo(int nID)
{
    int i;

    if ((unsigned int)(nID - 0x97) < 36) {
        for (i = 0; i < 36; i++) {
            if (nID == _srsChrEfTbl3[i].nID) {
                return _srsChrEfTbl3[i].nEftNo;
            }
        }
    }
    return 0;
}

/* --- effect id classification and name lookup --- */

/* Which of the eight effect-number bands an effect belongs to. Written
 * as one accumulator plus early `goto done` rather than eight returns:
 * every assignment to nType lands in the delay slot of the branch that
 * precedes it, which is only possible while there is a single exit. */
int srsGetEffectType(int nEftNo)
{
    int nType;

    nType = 0;
    if ((unsigned int)(nEftNo - 2600) < 62) goto done;
    nType = 2;
    if ((unsigned int)(nEftNo - 2500) < 100) goto done;
    if (nEftNo < 100) goto done;
    nType = 14;
    if ((unsigned int)(nEftNo - 2000) < 219) goto done;
    nType = 11;
    if ((unsigned int)(nEftNo - 240) < 157) goto done;
    nType = 14;
    if ((unsigned int)(nEftNo - 2300) < 182) goto done;
    nType = 2;
    if ((unsigned int)(nEftNo - 2800) < 200) goto done;
    nType = ((unsigned int)(nEftNo - 600) < 400) ? 15 : 14;
done:
    return nType;
}

/* NEAR-MISS (1 word short of 33, ~10 differing words). The original
 * keeps TWO separate return sites -- `jr ra / move v0,a3` inside the
 * loop for the hit, and `jr ra / li v0,-1` at the tail for both the
 * `nKey <= 0` guard and the loop falling out -- and leaves a nop in the
 * blez delay slot. Every shape tried lets gcc sink `li v0,-1` into that
 * delay slot and cross-jump the hit return into the same block, which
 * is exactly one word shorter. Swept: shared-exit accumulator with
 * goto; three `return i`s; block-local `short *` for each of the three
 * compared fields (in use order and in address order); index form vs
 * pointer form for the table walk; an early `if (nKey <= 0) return -1`
 * overshoots by one instead (34). The three hoisted base pointers
 * (tbl+2, tbl+4, tbl+6) come out right in every variant -- it is only
 * the return-block layout that resists.
 *
 * Weapon cross-reference search. The three compared ids sit at +2, +4
 * and +6 of each 8-byte record -- the record's leading nWeaponID is not
 * examined, which is why the induction variable anchors at
 * _weaponTbl+2 and gcc hoists three separate base pointers. */
int srsGetWeaponEffectIdx(int nEftNo)
{
    int nKey;
    int i;

    nKey = nEftNo;
    if ((unsigned int)(nEftNo - 2900) < 99) {
        nKey = nEftNo - 100;
    }
    if (nKey > 0) {
        for (i = 0; i < 45; i++) {
            if (nKey == _weaponTbl[i].nEffectID) return i;
            if (nKey == _weaponTbl[i].nAltID1) return i;
            if (nKey == _weaponTbl[i].nAltID2) return i;
        }
    }
    return -1;
}

int strcmp(const char *a, const char *b);

/* Reverse of srsGetEffectName: linear scan of the whole effect table
 * for a matching name. Effect 0 is skipped. */
int srsEffectNameToID(char *pName)
{
    int nRet;
    int i;

    if (pName == 0) {
        nRet = 0;
        goto done;
    }
    for (i = 1; i < 3072; i++) {
        char *p = (char *)srsGetEffectData(i);
        if (p != 0) {
            if (strcmp(pName, p) == 0) {
                nRet = i;
                goto done;
            }
        }
    }
    nRet = 0;
done:
    return nRet;
}

/* --- effect id -> table index bias --------------------------------- */

extern signed char _srsEffect2IndexTbl[];

/* NEAR-MISS (34 or 35 words vs 37, depending on arm shape). The
 * original returns through an accumulator parked in $a1 with three
 * separate `move v0,a1` return sites (one per arm shape: the table arm,
 * the first range arm, and the shared tail). Every source shape tried
 * either coalesces the accumulator into $v1 and returns directly (34
 * words, all arms `goto done`) or splits the returns but keeps the
 * value in $v0 (35 words, first two arms `return nIdx`). Swept: one
 * accumulator vs accumulator-plus-temp (gcc coalesces the copy away);
 * all-goto, all-return, and every mixed split of the six arms; the
 * final movz written as ?: and as if/else. A register-home tie-break.
 *
 * Maps an effect number onto the "second effect" index used to pick the
 * esd record: a byte table for the 601..717 band, and a set of small
 * offset bands above 2600. Single exit through one accumulator -- the
 * original's last two arms are a movz, which only forms while both
 * candidate values are live in the same block. */
int srsGetEffect2Idx(int nEftNo)
{
    int nIdx;

    if ((unsigned int)(nEftNo - 601) < 117) {
        return _srsEffect2IndexTbl[nEftNo - 601];
    }
    nIdx = nEftNo - 2600;
    if ((unsigned int)nIdx < 12) goto done;
    nIdx = nEftNo - 2612;
    if ((unsigned int)nIdx < 9) goto done;
    nIdx = nEftNo - 2651;
    if ((unsigned int)nIdx < 10) goto done;
    nIdx = nEftNo - 2626;
    if ((unsigned int)nIdx < 4) goto done;
    nIdx = nEftNo - 2630;
    if ((unsigned int)nIdx < 7) goto done;
    nIdx = ((unsigned int)(nEftNo - 2900) < 99) ? 100 : 0;
done:
    return nIdx;
}

/* PARKED at 12 diffs. Everything except the CD arm's two returns is
 * byte-exact: retail branches (`bgtzl s1` + `move v0,s1`, then `b` +
 * `li v0,-1`) where gcc if-converts the pair into `slt`/`movn` and, to
 * do it, keeps srsLoadMode alive in $s0 as a known zero -- which then
 * pushes `fd` off $s0 and reshuffles the host arm's schedule. The two
 * properties trade off and no source shape got both:
 *   - explicit `return -1;` after the `if` -> nLen correctly lands in
 *     the callee-saved $s1 (shared with the host arm) but gcc
 *     if-converts (12 diffs, this shape);
 *   - falling through to the shared final `return -1`, or `goto fail`,
 *     or `goto ret_len` -> no if-conversion and srsLoadMode correctly
 *     stays in $v1, but copy propagation coalesces the CD arm's nLen
 *     into $v0 and it becomes `blezl v0` (15 / 20 diffs).
 * Also swept: `if (nLen <= 0) return -1; return nLen;`, an explicit
 * `else { return -1; }`, declaration order of fd/nLen (this order is
 * right: fd->$s0, nLen->$s1), reading srsLoadMode directly instead of
 * through nMode, LAUNDER(nLen) before the test (no effect) and
 * LAUNDER_V inside the then-arm (50 words). Blocking the
 * if-conversion is the whole remaining job.
 *
 * Length of a sound-resource file, by whichever route srsLoadMode
 * selects: 0 = the CD image (path separators converted first),
 * 1 = the host filesystem (seek to end, seek back, close).
 * Any other mode -- and any failure -- reports -1. */
extern int srsMakeFileName(char *pName, int nFileNo);
extern int xglCdGetFileSize(char *pName);
extern int sceOpen(char *pName, int nFlags);
extern int sceLseek(int fd, int nOfs, int nWhence);
extern int sceClose(int fd);

int srsGetFileLen(int nFileNo)
{
    char szName[256];
    int nMode;
    int fd;
    int nLen;

    if (srsMakeFileName(szName, nFileNo) != 0) {
        nMode = srsLoadMode;
        if (nMode == 0) {
            srsChangeSeparator(szName);
            nLen = xglCdGetFileSize(szName);
            if (nLen > 0) {
                return nLen;
            }
            return -1;
        } else if (nMode == 1) {
            fd = sceOpen(szName, 1);
            if (fd >= 0) {
                nLen = sceLseek(fd, 0, 2);
                sceLseek(fd, 0, 0);
                sceClose(fd);
                return nLen;
            }
        }
    }
    return -1;
}

/* --- sres*: the resource-memory record.  Two blocks come out of the sm*
 * heap -- the common image (0x25800) and the cf image (0x3E000) -- and
 * each is handed to the image mapper through a stack slot. --- */

typedef struct
{
    void *pImage;           /* 0x00 */
    int   f0004;            /* 0x04 */
    int   f0008;            /* 0x08 */
} SRS_RELOAD;               /* 12 bytes */

typedef struct
{
    void       *pCommon;        /* 0x000 */
    int         f0004;          /* 0x004 */
    SRS_RELOAD  aReload[3];     /* 0x008 */
    void       *aBattle[3];     /* 0x02C */
    void       *pEvent;         /* 0x038 */
    void       *pCf;            /* 0x03C */
    void       *aMap[24];       /* 0x040 */
    char        pad00A0[0x54];  /* 0x0A0 */
    short       aReloadNo[9];   /* 0x0F4 */
    char        pad0106[0xA];   /* 0x106 */
    short       aMapNo[24];     /* 0x110 */
    void       *aMap2[24];      /* 0x140 */
} SRS_MEMRES;                   /* 0x1A0 */

extern SRS_MEMRES _srsMemRes;
extern char D_004CC6F0[];
extern char D_004CC700[];
extern char D_004CC710[];
extern void *memset(void *, int, unsigned int);
extern void *smAlloc(unsigned int nBytes);
extern void smFree(void *p);
extern void svAddImageMapper(int a, int b, void **pp, int n);
extern void svDeleteImageMapper(int n);
extern void sresFreeReloaderMemory(int n);

void sresInitMemoryRes(void)
{
    memset(&_srsMemRes, 0, sizeof(SRS_MEMRES));
}

/* SWEPT, 30 diffs, correct length (46): everything matches except how
 * gcc decomposes the in-range arm's address. Retail holds base+8 in $s2
 * and the element address in $s0, so the two follow-up zero stores CSE
 * to plain `move` copies of $s0 at offsets 4 and 8; gcc holds the bare
 * base in $s2 and CSEs `base + nNo*12` instead, so the same stores come
 * out at offsets 12 and 16 off a recomputed sum. A `q = &p->aReload[nNo]`
 * local removes the two moves entirely (44 words); decaying the array to
 * a `SRS_RELOAD *` folds the +8 into the relocation (40 words); the bare
 * symbol in both arms folds both (41 words). */
void sresFreeReloaderMemoryNo(int nNo)
{
    if (nNo < 3) {
        /* Through a pointer local (so the +8 stays a separate addiu off
         * the bare symbol) and naming the element on every store (so the
         * three zeroes go through CSE copies of the address, as retail's
         * move/move pair does). */
        SRS_MEMRES *p = &_srsMemRes;

        if (p->aReload[nNo].pImage != 0) {
            svDeleteImageMapper(nNo * 3 + 2);
            smFree(p->aReload[nNo].pImage);
            p->aReload[nNo].pImage = 0;
            p->aReload[nNo].f0004 = 0;
            p->aReload[nNo].f0008 = 0;
        }
    } else {
        /* The bare symbol here: retail folds +0x20 into the relocation. */
        void **pp = &_srsMemRes.aBattle[nNo - 3];

        if (*pp != 0) {
            svDeleteImageMapper(nNo + 8);
            smFree(*pp);
            *pp = 0;
        }
    }
}

void sresLoadCommonMemory(void)
{
    SRS_MEMRES *p = &_srsMemRes;
    void *pTmp[8];
    void *pMem;
    int n;

    if (p->pCommon != 0) {
        return;
    }
    pMem = smAlloc(0x25800);
    p->pCommon = pMem;
    if (pMem != 0) {
        n = fileLoad((int)pMem, (int)D_004CC6F0, 0);
        /* Two separate tests, not `n >= 0 && n <= K`: written as one
         * expression gcc folds the pair into a single unsigned compare
         * and loses retail's bltz. */
        if (n >= 0) {
            if (n <= 0x25800) {
                pTmp[0] = p->pCommon;
                svAddImageMapper(0, 0, pTmp, 3000);
            }
        }
    }
    memset(_loadEsdData, 0, 0xC800);
    fileLoad((int)_loadEsdData, (int)D_004CC700, 0);
}

void sresLoadCfMemory(void)
{
    SRS_MEMRES *p = &_srsMemRes;
    void *pTmp[8];
    void *pMem;

    if (p->pCf != 0) {
        return;
    }
    pMem = smAlloc(0x3E000);
    p->pCf = pMem;
    if (pMem == 0) {
        return;
    }
    if (fileLoad((int)pMem, (int)D_004CC710, 0) < 0) {
        return;
    }
    pTmp[0] = p->pCf;
    svAddImageMapper(15, 0, pTmp, 3500);
}

void sresFreeMemoryRes(void)
{
    SRS_MEMRES *p;

    sresFreeReloaderMemory(1);
    p = &_srsMemRes;
    if (p->pCommon != 0) {
        svDeleteImageMapper(0);
        smFree(p->pCommon);
        p->pCommon = 0;
    }
}

