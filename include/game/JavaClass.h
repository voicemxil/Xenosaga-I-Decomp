/* Script-VM class/constant-pool layouts.
 *
 * Recovered from loadConstString (string interner, 0x002f1860) and
 * lookupClassField (field-table binary-ish linear search, 0x002f5be0),
 * the two highest fan-in unmatched primitives in the game (197 and 181
 * callers respectively). Every Java_*.c native reaches class/field data
 * through these two calls, so the struct layouts below are load-bearing
 * for the whole family even though most of each struct's fields are
 * still unknown (marked as raw padding).
 */
#ifndef GAME_JAVACLASS_H
#define GAME_JAVACLASS_H

/* --- Interned-string table (loadConstString) ---
 * A single global 512-bucket hash table of interned (char*,length)
 * strings, chained on collision. Callers pass nLength == -1 to mean
 * "compute via strlen"; nLength == 0 is rejected (returns NULL). The
 * returned STRENTRY* is itself the "name id" that lookupClassField's
 * second argument compares by pointer identity -- interning makes
 * field/method name lookup a pointer compare instead of a strcmp. */
typedef struct STRENTRY
{
    struct STRENTRY *next;   /* 0x00 - next entry in this bucket's chain */
    unsigned short    hash;  /* 0x04 - bucket index (hash & 0x1FF), not the raw hash */
    unsigned short    length;/* 0x06 - interned string length */
    char             *data;  /* 0x08 - pointer to the (uncopied) source bytes */
} STRENTRY;                  /* size 0x0C, alloc tag 20 (see StringUtf_create's tag) */

#define STRTAB_SIZE 512

/* --- Class field table (lookupClassField) ---
 * Only the parts touched by lookupClassField are recovered; the rest of
 * the class record (superclass, methods, vtable, etc.) is still opaque
 * and left as raw padding here on purpose -- do not treat pad sizes as
 * verified beyond "this many bytes exist before the next known field". */
typedef struct JFIELD
{
    void *name;          /* 0x00 - interned string pointer (STRENTRY*), compared by identity */
    char  pad04[0x0C - 0x04]; /* 0x04 - unknown */
    unsigned int nSize;  /* 0x0C - declared byte size.  allocStaticField reuses this slot
                                   as a scratch offset while it lays the static block out,
                                   then restores it from nOffset. */
    int   nOffset;       /* 0x10 - byte offset within the instance; for static fields
                                   allocStaticField overwrites it with the absolute
                                   address inside the freshly allocated static block. */
} JFIELD;                /* size 0x14 (20), confirmed via the *5*4 index-scaling in lookupClassField */

/* A method table entry, keyed by the interned name/signature pair. */
typedef struct JMETHOD
{
    void *pName;         /* 0x00 */
    void *pSig;          /* 0x04 */
} JMETHOD;

typedef struct JCLASS
{
    char pad00[0x04];                 /* 0x00 - unknown */
    void *pName;                      /* 0x04 - interned class name */
    char pad08[0x0A - 0x08];          /* 0x08 - unknown */
    unsigned short nFlags;            /* 0x0A - 0x100 marks a primitive/wrapper class */
    char pad0C[0x10 - 0x0C];          /* 0x0C - unknown */
    struct JCLASS *pSuper;            /* 0x10 */
    int *pConst;                      /* 0x14 - constant pool; slot 0 points at the
                                                parallel array of one-byte tags */
    int pType;                        /* 0x18 - the word every instance of this class
                                                carries in its first slot; newClass
                                                inherits it from classClass.  Typed int,
                                                not void*: with -fstrict-aliasing a void*
                                                load may be hoisted above the int stores
                                                that precede it in newClass, and the
                                                original build did not hoist it. */
    JFIELD *fields;                   /* 0x1C - base of the field table */
    JMETHOD *pMethods;                /* 0x20 - base of the method table */
    char pad24[0x28 - 0x24];          /* 0x24 - unknown */
    unsigned short nConstCount;       /* 0x28 - entries in pConst */
    char pad2A[0x2C - 0x2A];          /* 0x2A - unknown */
    unsigned short nMethodCount;      /* 0x2C - entries in pMethods */
    char pad2E[0x30 - 0x2E];          /* 0x2E - unknown */
    unsigned short nFieldCount;       /* 0x30 - total entries in `fields` */
    unsigned short nStaticFieldCount; /* 0x32 - static fields occupy fields[0..nStaticFieldCount) */
    char pad34[0x38 - 0x34];          /* 0x34 - unknown */
    /* 0x38 - a reference class stores the instance byte size newObject hands
       to xmalloc; a primitive/wrapper class (nFlags & 0x100) instead uses the
       middle two bytes for its signature letter and element width, and
       newClass zeroes the whole word. */
    union
    {
        int nInstanceSize;
        struct
        {
            unsigned char pad38;
            signed char   cSigChar;   /* 0x39 */
            unsigned char nElemSize;  /* 0x3A */
            unsigned char pad3B;
        } prim;
    } u;
    char pad3C[0x40 - 0x3C];          /* 0x3C - unknown; newClass allocates 0x40 */
} JCLASS;

extern void *loadConstString(char *pStr, int nLength);
extern JFIELD *lookupClassField(JCLASS *pClass, void *pName, int nFlags);

#endif /* GAME_JAVACLASS_H */
