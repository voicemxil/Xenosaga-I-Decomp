/* Ether - the ether/skill tree UI system: a fixed-size object table
 * (EtherTreeObject) walked by a cursor (EtherTreeObjectP), plus the line,
 * line2 and right-panel work buffers that hang off the same system block.
 */

typedef struct {
    unsigned short id;          /* 0x00 */
    char pad02[0x06];            /* 0x02 */
    unsigned short lineType;    /* 0x08 */
    char pad0A[0x0E];           /* 0x0A */
    unsigned char flags;         /* 0x18 */
    unsigned char pad19;         /* 0x19 */
    unsigned char category;     /* 0x1A */
    char pad1B[0x05];           /* 0x1B */
    float x;                     /* 0x20 */
    float y;                     /* 0x24 */
    float x2;                    /* 0x28 */
    float y2;                    /* 0x2C */
    char pad30[0x15];            /* 0x30 */
    unsigned char unlocked;     /* 0x45 */
    char pad46[0x02];           /* 0x46 */
    unsigned short value48;     /* 0x48 */
    char pad4A[0x26];           /* 0x4A */
} ETHER_OBJECT;

typedef struct {
    unsigned char flags;        /* 0x00 */
    char pad01;                 /* 0x01 */
    unsigned short wId;         /* 0x02 */
    char pad04[0x0C];           /* 0x04 */
    float fCenterX;             /* 0x10 */
    float fCenterY;             /* 0x14 */
    char pad18[0x08];           /* 0x18 */
    float fToCenterX;           /* 0x20 */
    float fToCenterY;           /* 0x24 */
    char pad28[0x08];           /* 0x28 */
    float fBaseX;               /* 0x30 */
    float fBaseY;               /* 0x34 */
    char pad38[0x08];           /* 0x38 */
    float fTargetX;             /* 0x40 */
    float fTargetY;             /* 0x44 */
    char pad48[0x08];           /* 0x48 */
    ETHER_OBJECT *target;       /* 0x50 */
    char pad54[0x1C];           /* 0x54 */
    void *windowTexture;        /* 0x70 */
} ETHER_SYSTEM;

typedef struct {
    unsigned char flags;        /* 0x00 */
    unsigned char mode;         /* 0x01 */
    unsigned short timer;       /* 0x02 */
} ETHER_BLACK;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} ETHER_VEC4;

typedef struct {
    unsigned char flags;        /* 0x00 */
    unsigned char mode;         /* 0x01 */
    unsigned char state;        /* 0x02 */
    unsigned char type;         /* 0x03 */
    int selection;              /* 0x04 */
    char pad08[0x08];           /* 0x08 */
    ETHER_VEC4 base[3];         /* 0x10 */
    ETHER_VEC4 position[3];     /* 0x40 */
    ETHER_OBJECT *target;       /* 0x70 */
    char pad74[0xCC];           /* 0x74 */
} ETHER_LINE2;

typedef struct {
    unsigned char flags;        /* 0x00 */
    unsigned char mode;         /* 0x01 */
    char pad02[0x0E];           /* 0x02 */
    unsigned long long position0; /* 0x10 */
    unsigned long long position1; /* 0x18 */
    ETHER_OBJECT *target;       /* 0x20 */
    char pad24[0x08];           /* 0x24 */
    signed char color0;         /* 0x2C */
    signed char color1;         /* 0x2D */
    signed char color2;         /* 0x2E */
    unsigned char alpha0;       /* 0x2F */
    int field30;                /* 0x30 */
    int field34;                /* 0x34 */
    signed char color3;         /* 0x38 */
    signed char color4;         /* 0x39 */
    signed char color5;         /* 0x3A */
    unsigned char alpha1;       /* 0x3B */
    char pad3C[0x04];           /* 0x3C */
} ETHER_RIGHT;

typedef struct {
    ETHER_OBJECT *object;       /* 0x00 */
    char pad04[0x18C];          /* 0x04 */
} ETHER_LINE;

extern ETHER_OBJECT *EtherTreeObject;
extern ETHER_OBJECT *EtherTreeObjectP;
extern ETHER_SYSTEM *EtherTreeSystem;
extern ETHER_LINE *EtherTreeLine;
extern ETHER_LINE2 *EtherTreeLine2;
extern ETHER_RIGHT *EtherTreeRight;
extern ETHER_BLACK *EtherTreeBlack;
extern int EtherTreeFirstData[];

extern void *memset(void *pDst, int c, unsigned int n);
extern void subLine2_DrawType_1(ETHER_LINE2 *line);
extern void subEtherTreeRightMain(ETHER_RIGHT *right);
extern void subRightDraw(ETHER_RIGHT *right);
extern void *WindowTexAddrGet(int index);
extern void endPrintExtFunc(int x, int type, void *data);
extern void EtherTreeObjectDraw(void);
extern void EtherTreeLineDraw(void);
extern void EtherTreeCursolDraw(void);
extern void EtherTreeBlackDraw(void);
extern void EtherTreeLine2ModeChange(int mode);
extern void subMoveSlide(float *value, float *target, float amount);
extern void subTreeLineDraw_type_0(ETHER_LINE *line);
extern void subTreeLineDraw_type_1(ETHER_LINE *line);
extern void subTreeLineDraw_type_2(ETHER_LINE *line);
extern void subTreeLineDraw_type_3(ETHER_LINE *line);
extern int MenuEtherSetCheck(int treeId, unsigned int objectId);
extern int func_A19578(int treeId, unsigned int objectId);
extern int MenuEtherTypeGet(unsigned int objectId);

/* Find the object with the given id (linear scan over the fixed 80-slot table) */
ETHER_OBJECT *EtherTreeObjectGet(unsigned int id)
{
    ETHER_OBJECT *p = EtherTreeObject;
    int i = 0;
    do {
        unsigned short v = p->id;
        if (v == id) {
            return p;
        }
        p++;
        i++;
    } while (i < 80);
    return 0;
}

/* Reset the object cursor to the start of the table */
ETHER_OBJECT *EtherTreeObjectGetClear(void)
{
    return EtherTreeObjectP = EtherTreeObject;
}

/* Hand out the current cursor slot and advance it */
ETHER_OBJECT *EtherTreeObjectWorkGet(void)
{
    ETHER_OBJECT *ret;

    ret = EtherTreeObjectP;
    EtherTreeObjectP = (ETHER_OBJECT *)((char *)ret + 0x70);
    return ret;
}

/* Look up the first-data table entry for the current ether id */
void *EtherTreeFirstDataGet(void)
{
    return &EtherTreeFirstData[EtherTreeSystem->wId - 1];
}

/* Zero every ether-tree work buffer */
void EtherTreeWorkClear(void)
{
    memset(EtherTreeSystem, 0, 0x80);
    memset(EtherTreeObject, 0, 0x2300);
    memset(EtherTreeLine, 0, 0x1900);
    memset(EtherTreeLine2, 0, 0x140);
    memset(EtherTreeRight, 0, 0x80);
}

/* Set the ether tree's destination center */
void EtherTreeToCenterSet(float x, float y)
{
    EtherTreeSystem->fToCenterX = x;
    EtherTreeSystem->fToCenterY = y;
}

/* Reset the black-overlay state */
void EtherTreeBlackSet(void)
{
    ETHER_BLACK *p = EtherTreeBlack;

    p->mode = 0;
    p->timer = 0;
}

/* TODO: near-match (LENGTH) - the natural switch preserves 32 of the original
 * 34 instructions, but gcc folds the final two guards into one likely branch. */
/* Change the black-overlay mode when the transition is permitted */
void EtherTreeBlackModeChange(int mode)
{
    ETHER_BLACK *black = EtherTreeBlack;

    if ((EtherTreeSystem->flags & 1) != 0) {
        switch (black->mode) {
        case 0:
            black->mode = mode;
            break;
        case 1:
        case 2:
            if (mode != 1 && mode != 2) {
                black->mode = mode;
            }
            break;
        case 3:
            if (mode == black->mode) {
                break;
            }
            if (mode == 0) {
                break;
            }
            black->mode = mode;
            break;
        }
    }
}

/* Copy one of the four ether-line colors */
void EtherTreeLineColorGet(unsigned char *color, int type)
{
    unsigned char colors[4][4] = {
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x40, 0x40, 0x40, 0x60 },
        { 0x80, 0x80, 0x80, 0x80 },
        { 0xA0, 0xA0, 0x40, 0x80 }
    };
    int i;

    for (i = 0; i < 4; i++) {
        color[i] = colors[type][i];
    }
}

/* TODO: near-match - the natural local pointer remains in $v1 and is copied
 * to $a0 for the call; the original allocates it directly in $a0. */
/* Draw the secondary ether-tree line layer when active */
void EtherTreeLine2Draw(void)
{
    if ((EtherTreeSystem->flags & 1) != 0) {
        ETHER_LINE2 *line = EtherTreeLine2;

        if ((line->flags & 1) != 0) {
            subLine2_DrawType_1(line);
        }
    }
}

/* Update both ether-tree right-panel slots */
void EtherTreeRightMain(void)
{
    int i;

    if ((EtherTreeSystem->flags & 1) != 0) {
        for (i = 0; i < 2; i++) {
            subEtherTreeRightMain(&EtherTreeRight[i]);
        }
    }
}

/* Change one right-panel slot's selected ether object */
void EtherTreeRightTargetChange(unsigned int id, int index)
{
    ETHER_RIGHT *right = &EtherTreeRight[index];

    if ((EtherTreeSystem->flags & 1) != 0 && (right->flags & 1) != 0) {
        right->target = EtherTreeObjectGet(id);
    }
}

/* Collect visible ether objects into the primary line-work list */
void EtherTreeLineSet(void)
{
    ETHER_LINE *line = EtherTreeLine;
    ETHER_OBJECT *object;
    int i;

    if ((EtherTreeSystem->flags & 1) != 0) {
        object = EtherTreeObject;
        for (i = 0; i < 0x50; i++, object++) {
            if ((object->flags & 4) != 0 && object->id != 0) {
                line->object = object;
                line++;
            }
        }
    }
}

/* Select an ether object as the tree's movement target */
void EtherTreeTargetChange(unsigned int id, int setBase)
{
    ETHER_OBJECT *object = EtherTreeObjectGet(id);
    ETHER_SYSTEM *system = EtherTreeSystem;

    if ((system->flags & 1) != 0 && object != 0) {
        system->target = object;
        system->fTargetX = -object->x;
        system->fTargetY = -object->y;
        if (setBase == 1) {
            system->fBaseX = system->fTargetX;
            system->fBaseY = system->fTargetY;
        }
    }
}

/* TODO: near-match (LOGIC) - one opcode differs: gcc emits beqzl for the
 * mode test while the original uses beqz. Root cause matches the
 * ACT_setArms wall: the mode-check's delay slot (the loop's "i < 2" test)
 * is dead on the not-taken/fallthrough path (subRightDraw's call site
 * recomputes it right after returning), so 2.96 uses the annulling branch
 * to skip it there; the original just re-executes it redundantly. The
 * flags-check branch just above stays plain beqz because ITS delay slot is
 * the i++ increment, which is never dead. Tried: continue-style early-exit
 * (identical codegen to &&), an asm barrier on `i` before the call (made it
 * worse, 4 diffs). Same class of unreachable-from-C annulment heuristic as
 * ACT_setArms; do not re-attempt without a new idiom for that wall. */
/* Draw each active ether-tree right-panel slot */
void EtherTreeRightDraw(void)
{
    int i;

    if ((EtherTreeSystem->flags & 1) != 0) {
        for (i = 0; i < 2; i++) {
            ETHER_RIGHT *right = &EtherTreeRight[i];

            if ((right->flags & 1) != 0 && right->mode != 0) {
                subRightDraw(right);
            }
        }
    }
}


/* Move the ether-tree center unless the requested axis is already settled */
void EtherTreeCenterSet(int mode, float x, float y)
{
    ETHER_SYSTEM *system = EtherTreeSystem;

    if ((system->flags & 1) == 0) {
        return;
    }
    if ((mode & 1) != 0 && system->fCenterX == system->fToCenterX) {
        return;
    }
    if ((mode & 2) != 0 && system->fCenterY == system->fToCenterY) {
        return;
    }
    system->fCenterX = x;
    system->fCenterY = y;
    system->flags |= 2;
}

/* Lay out all ether-tree work areas in a caller-provided arena */
void *EtherTreeInit(void *memory)
{
    char *p = (char *)(((unsigned int)memory + 0xF) & ~0xF);

    EtherTreeSystem = (ETHER_SYSTEM *)p;
    p += 0x80;
    EtherTreeObject = (ETHER_OBJECT *)p;
    p += 0x2300;
    EtherTreeLine = (ETHER_LINE *)p;
    p += 0x1900;
    EtherTreeLine2 = (ETHER_LINE2 *)p;
    p += 0x140;
    EtherTreeRight = (ETHER_RIGHT *)p;
    p += 0x80;
    EtherTreeBlack = (ETHER_BLACK *)p;
    p += 0x40;

    EtherTreeWorkClear();
    EtherTreeBlackSet();
    return p;
}

/* Draw every active ether-tree layer */
void EtherTreeDraw(void)
{
    if ((EtherTreeSystem->flags & 1) != 0) {
        EtherTreeSystem->windowTexture = WindowTexAddrGet(2);
        endPrintExtFunc(0, 0xE, &EtherTreeSystem->windowTexture);
        EtherTreeObjectDraw();
        EtherTreeLineDraw();
        EtherTreeLine2Draw();
        EtherTreeCursolDraw();
        EtherTreeRightDraw();
        EtherTreeBlackDraw();
        endPrintExtFunc(0, 0xF, 0);
    }
}

/* TODO: near-match (LOGIC) - the state-machine behavior is reconstructed,
 * but the original's comparison tree and epilogue schedule are not matched. */
/* Advance the ether-tree black-overlay fade */
void EtherTreeBlackMain(void)
{
    ETHER_BLACK *black;
    int mode;

    if ((EtherTreeSystem->flags & 1) == 0) {
        return;
    }
    black = EtherTreeBlack;
    mode = black->mode;
    if (mode == 1) {
        int timer = black->timer + 8;

        black->timer = timer;
        if ((short)timer < 0x50) {
            return;
        }
        black->mode = 2;
        black->timer = 0x50;
        return;
    }
    if (mode < 2) {
        return;
    }
    if (mode == 2) {
        black->timer = 0x50;
        return;
    }
    if (mode == 3) {
        int timer = black->timer;

        timer -= 8;
        black->timer = timer;
        if ((short)timer > 0) {
            return;
        }
        black->mode = 0;
        black->timer = 0;
    }
}

/* TODO: near-match (LOGIC) - three words differ: gcc makes the inactive-line
 * guard likely and fills the equal-selection delay slot with the epilogue load,
 * while the original uses a plain guard and loads mode 1 in that delay slot. */
/* Change the selected secondary-line item, reopening it when necessary */
void EtherTreeLine2SelectChange(int selection)
{
    ETHER_LINE2 *line = EtherTreeLine2;

    if ((EtherTreeSystem->flags & 1) != 0) {
        if ((line->flags & 1) != 0) {
            if (line->selection != selection) {
                line->selection = selection;
                EtherTreeLine2ModeChange(0);
            }
            EtherTreeLine2ModeChange(1);
        }
    }
}

/* Slide both center pairs toward their active targets */
void EtherTreeCenterMove(void)
{
    ETHER_SYSTEM *system = EtherTreeSystem;
    float *center = &system->fCenterX;
    float *toCenter = &system->fToCenterX;
    float *base = &system->fBaseX;
    float *target = &system->fTargetX;

    if ((system->flags & 1) != 0 && (system->flags & 2) != 0) {
        subMoveSlide(center, toCenter, 0.4f);
        subMoveSlide(center + 1, toCenter + 1, 0.4f);
        subMoveSlide(base, target, 0.4f);
        subMoveSlide(base + 1, target + 1, 0.4f);
    }
}

/* TODO: near-match (LENGTH) - 40 instructions versus 42 original; gcc folds
 * the two final case-3 rejection guards into one bnezl/store sequence. */
/* Change one right-panel mode when its current transition allows it */
void EtherTreeRightModeChange(int mode, int index)
{
    ETHER_RIGHT *right = &EtherTreeRight[index];

    if ((EtherTreeSystem->flags & 1) != 0) {
        if ((right->flags & 1) != 0) {
            switch (right->mode) {
            case 0:
                right->mode = mode;
                break;
            case 1:
            case 2:
                if (mode != 1 && mode != 2) {
                    right->mode = mode;
                }
                break;
            case 3:
                if (mode == right->mode) {
                    break;
                }
                if (mode == 0) {
                    break;
                }
                right->mode = mode;
                break;
            }
        }
    }
}

/* TODO: near-match (LENGTH) - 40 instructions versus 41 original; gcc shares
 * the two literal 0x20 values and rotates the final independent byte stores,
 * unlike the original's two li instructions and store schedule. */
/* Initialize one right-panel slot from an ether object */
void EtherTreeRightSet(unsigned int id, int index)
{
    ETHER_RIGHT *right = &EtherTreeRight[index];

    if ((EtherTreeSystem->flags & 1) != 0) {
        ETHER_OBJECT *object = EtherTreeObjectGet(id);

        right->target = object;
        if (object != 0) {
            right->flags |= 1;
            right->mode = 0;
            right->field34 = 0x20;
            right->field30 = 0;
            right->position0 = *(unsigned long long *)&object->x;
            right->position1 = *(unsigned long long *)&object->x2;
            right->alpha0 = 0x20;
            right->color3 = -0x80;
            right->alpha1 = 0;
            right->color2 = -0x80;
            right->color1 = -0x80;
            right->color0 = -0x80;
            right->color5 = -0x80;
            right->color4 = -0x80;
        }
    }
}

/* TODO: near-match (LENGTH) - 33 instructions versus 35 original; gcc moves
 * the EtherTreeLine load after the static dispatch-table address and omits the
 * original alignment nop immediately before the loop. */
/* Dispatch each active primary line through its line-type renderer */
void EtherTreeLineDraw(void)
{
    ETHER_LINE *line = EtherTreeLine;
    void (*draw[4])(ETHER_LINE *) = {
        subTreeLineDraw_type_0,
        subTreeLineDraw_type_1,
        subTreeLineDraw_type_2,
        subTreeLineDraw_type_3
    };
    int i;

    for (i = 15; i >= 0; i--, line++) {
        if (line->object != 0) {
            draw[line->object->lineType](line);
        }
    }
}

/* TODO: near-match (LENGTH) - 54 instructions versus 53 original; gcc hoists
 * the target/count work to the loop header, while the original reloads the
 * target between the position and base copies and tests the count at the tail. */
/* Seed the secondary-line points from an ether object */
void EtherTreeLine2Set(unsigned int id, int type)
{
    ETHER_LINE2 *line = EtherTreeLine2;

    if ((EtherTreeSystem->flags & 1) != 0) {
        int i;

        line->target = EtherTreeObjectGet(id);
        for (i = 0; i < line->target->lineType; i++) {
            line->position[i] = *(ETHER_VEC4 *)&line->target->x;
            line->base[i] = line->position[i];
        }
        line->flags |= 1;
        line->type = type;
        line->state = 0;
        line->mode = 0;
    }
}

/* TODO: near-match (LENGTH) - 58 instructions versus 59 original; all unlock
 * logic matches, but gcc folds the final signed category selection to movz
 * instead of preserving the original bltz/slti/beql branch tree. */
/* Refresh one ether object's unlock and category parameters */
void EtherTreeParaSet(ETHER_OBJECT *object)
{
    ETHER_SYSTEM *system = EtherTreeSystem;
    int treeId = system->wId;

    if (object->id == 0) {
        return;
    }
    if ((object->flags & 4) != 0) {
        object->flags |= 8;
    }
    if (MenuEtherSetCheck(treeId, object->id) != 0) {
        object->unlocked = 1;
        object->value48 = 8;
        object->flags |= 2;
    } else {
        object->flags &= 0xFD;
    }
    if (func_A19578(treeId, object->id) != 0) {
        object->flags |= 9;
        object->flags &= 0xEF;
    } else if ((object->flags & 4) != 0) {
        object->flags |= 0x10;
    }
    {
        int type = MenuEtherTypeGet(object->id);

        if (type != 0) {
            unsigned char category;

            if (type < 0) {
                category = 2;
            } else if (type < 3) {
                category = 1;
            } else {
                category = 2;
            }
            object->category = category;
        } else {
            object->category = 0;
        }
    }
}
