/* Ether - the ether/skill tree UI system: a fixed-size object table
 * (EtherTreeObject) walked by a cursor (EtherTreeObjectP), plus the line,
 * line2 and right-panel work buffers that hang off the same system block.
 */

typedef struct {
    unsigned short id;          /* 0x00 */
    char pad02[0x6E];
} ETHER_OBJECT;

typedef struct {
    unsigned char flags;        /* 0x00 */
    char pad01;                 /* 0x01 */
    unsigned short wId;         /* 0x02 */
} ETHER_SYSTEM;

extern ETHER_OBJECT *EtherTreeObject;
extern ETHER_OBJECT *EtherTreeObjectP;
extern ETHER_SYSTEM *EtherTreeSystem;
extern void *EtherTreeLine;
extern void *EtherTreeLine2;
extern void *EtherTreeRight;
extern int EtherTreeFirstData[];

extern void *memset(void *pDst, int c, unsigned int n);

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
