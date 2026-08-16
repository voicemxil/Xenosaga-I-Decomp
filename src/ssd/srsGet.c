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
    short pad[2];      /* 0x04 */
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
