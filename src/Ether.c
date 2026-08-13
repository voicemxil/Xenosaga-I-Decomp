/* Ether - the ether/skill tree UI system: a fixed-size object table
 * (EtherTreeObject) walked by a cursor (EtherTreeObjectP), plus the line,
 * line2 and right-panel work buffers that hang off the same system block.
 */

typedef struct {
    unsigned short id;          /* 0x00 */
    char pad02[0x16];            /* 0x02 */
    unsigned char flags;         /* 0x18 */
    char pad19[0x07];            /* 0x19 */
    float x;                     /* 0x20 */
    float y;                     /* 0x24 */
    char pad28[0x48];            /* 0x28 */
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
    unsigned char flags;        /* 0x00 */
    unsigned char mode;         /* 0x01 */
    char pad02[0x02];           /* 0x02 */
    int selection;              /* 0x04 */
} ETHER_LINE2;

typedef struct {
    unsigned char flags;        /* 0x00 */
    unsigned char mode;         /* 0x01 */
    char pad02[0x1E];           /* 0x02 */
    ETHER_OBJECT *target;       /* 0x20 */
    char pad24[0x1C];           /* 0x24 */
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

/* TODO: near-match - allocator swaps p/v between $v1/$a1 and emits bnez
 * where the original has bnezl; every natural variant (declaration order,
 * unused second parameter to steal $a1) gives the same allocation, so this
 * looks like the same class of unreachable-from-C tie-break documented for
 * SsdGetMemoryBlocks / xglDmaMFIFOLeave. Not registered in decompiled.txt. */
/* Find the object with the given id, or NULL if the table has none */
ETHER_OBJECT *EtherTreeObjectGet(unsigned int id)
{
    ETHER_OBJECT *p = EtherTreeObject;
    int i = 0;
    unsigned short v = p->id;

    do {
        if (v == id) {
            return p;
        }
        p++;
        i++;
        v = p->id;
    } while (i < 0x50);
    return 0;
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

/* TODO: near-match (LENGTH) - simplified transition guards compile to 26
 * instructions while the original preserves a 34-instruction branch tree. */
/* Change the black-overlay mode when the transition is permitted */
void EtherTreeBlackModeChange(int mode)
{
    ETHER_BLACK *black;
    int current;

    if ((EtherTreeSystem->flags & 1) == 0) {
        return;
    }
    black = EtherTreeBlack;
    current = black->mode;
    if (current >= 3) {
        if (current != 3 || mode == 3 || mode == 0) {
            return;
        }
        black->mode = mode;
        return;
    }
    if (current != 0 && (unsigned int)(mode - 1) < 2) {
        return;
    }
    black->mode = mode;
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
 * mode test while the original uses beqz with the loop test in its slot. */
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

/* Change the selected secondary-line item, reopening it when necessary */
void EtherTreeLine2SelectChange(int selection)
{
    ETHER_LINE2 *line;

    if ((EtherTreeSystem->flags & 1) == 0) {
        return;
    }
    line = EtherTreeLine2;
    if ((line->flags & 1) == 0) {
        return;
    }
    if (line->selection != selection) {
        line->selection = selection;
        EtherTreeLine2ModeChange(0);
    }
    EtherTreeLine2ModeChange(1);
}
