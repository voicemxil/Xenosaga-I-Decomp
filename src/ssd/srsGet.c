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
 * pointer form for the table walk. The three hoisted base pointers
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
