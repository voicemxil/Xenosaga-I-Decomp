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
void *srsGetComboData(int idx)
{
    void *ret;

    ret = 0;
    if (idx < 0xC3) {
        if (_loadComboData[idx] != 0) {
            ret = &_loadComboData[idx];
        }
    }
    return ret;
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
