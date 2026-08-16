/* Battle-scene visual effect resource manager (sv* family).
 *
 * The image mapper is a fixed array of 40 "type lists", each one a
 * 0x1C-byte header followed by 256 image slots, and each one owning a
 * fixed arena of GS video memory (see the siz/top tables below).  A
 * loaded effect resource is a chain of chunks; svAnalyzeChunk walks it
 * and turns every chunk into an image slot -- CLUT, texture, model or
 * script -- bump-allocating GS memory out of the list's arena as it
 * goes.  svLoadImageList then DMAs the textures and CLUTs of one list
 * into that memory.
 */

#include "matching.h"

/* One image slot. 256 of these follow every type-list header. */
typedef struct {
    /* 0x00 */ void *data;              /* payload, inside the chunk */
    /* 0x04 */ int unk04;
    /* 0x08 */ int used;                /* 0 = free slot */
    /* 0x0C */ short kind;              /* 3 clut, 4 image, 5 script, 6 model */
    /* 0x0E */ unsigned short attr;     /* low byte is svGetImageListItem's key */
    /* 0x10 */ short unk10;
    /* 0x12 */ unsigned short tbp;      /* GS base pointer, vram word addr >> 6 */
    /* 0x14 */ unsigned short bpp;      /* 4 or 8 */
    /* 0x16 */ unsigned short tbw;      /* GS transfer width, in texels */
    /* 0x18 */ unsigned short w;
    /* 0x1A */ unsigned short h;
    /* 0x1C */ unsigned short wbit;     /* log2 w */
    /* 0x1E */ unsigned short hbit;     /* log2 h */
    /* 0x20 */ char *name;
} SV_IMAGE;

/* One type list: header + slots, 0x241C bytes. */
typedef struct {
    /* 0x00 */ int unk00;
    /* 0x04 */ unsigned int vramBase;   /* GS arena, in 32-bit words */
    /* 0x08 */ unsigned int vramSize;
    /* 0x0C */ unsigned int vramPtr;    /* bump allocator into the arena */
    /* 0x10 */ short id;
    /* 0x12 */ short data;
    /* 0x14 */ short num;               /* nonzero once a resource is bound */
    /* 0x16 */ short unk16;
    /* 0x18 */ int unk18;
    /* 0x1C */ SV_IMAGE item[256];
} SV_IMAGE_LIST;

/* Trailing per-list reference-image slot, one per type list. */
typedef struct {
    /* 0x00 */ int unk00;
    /* 0x04 */ int unk04;
} SV_REF;

/* The reference table reached off a register holding _imageMapper. The
 * original forms the large offset at run time rather than folding it
 * into %hi/%lo, which only happens when the base is a pointer value. */
#define SV_REFP(base) ((SV_REF *)((base) + 40 * sizeof(SV_IMAGE_LIST)))

/* Resource chunk header. The payload starts at +0x40 (+0xC0 for the
 * second script flavour); the chunk name is embedded at +0x14. */
typedef struct {
    /* 0x00 */ int size;                /* distance to the next chunk */
    /* 0x04 */ int dataSize;
    /* 0x08 */ short type;              /* 0 terminates the chain */
    /* 0x0A */ unsigned short attr;
    /* 0x0C */ short w;
    /* 0x0E */ unsigned short h;
    /* 0x10 */ short bpp;
    /* 0x12 */ short unk12;
    /* 0x14 */ char name[44];
} SV_CHUNK;

extern char _imageMapper[];
#define svImageMapper ((SV_IMAGE_LIST *)_imageMapper)
#define svRefImage ((SV_REF *)(_imageMapper + 40 * sizeof(SV_IMAGE_LIST)))

extern SV_IMAGE_LIST _imageList;

extern void *memset(void *, int, unsigned int);
extern int strcmp(const char *, const char *);
extern int strlen(const char *);
extern int strncmp(const char *, const char *, int);
extern void *xglPacketGetCurrent(void);
extern void sceVif1PkRefLoadImage(void *pkt, unsigned int dbp,
                                  unsigned int dpsm, unsigned int dbw,
                                  unsigned int addr, unsigned int qwc,
                                  unsigned int dsax, unsigned int dsay,
                                  unsigned int rrw, unsigned int rrh);

void *svGetTypeList(int idx);
void svImageListDestroy(SV_IMAGE_LIST *p);
int svImageListAlloc(SV_IMAGE_LIST *p);
int svGetSizeBit(int size);
SV_IMAGE *svGetImageListItemSub(int idx, int key);
void svLoadTexture(SV_IMAGE *p);
void svLoadClut(SV_IMAGE *p);
int svLoadImageList(SV_IMAGE_LIST *l);
int svAddImage(SV_IMAGE_LIST *l, SV_CHUNK *c);
int svAddClut(SV_IMAGE_LIST *l, SV_CHUNK *c);
int svAddModel(SV_IMAGE_LIST *l, SV_CHUNK *c);
int svAddScript(SV_IMAGE_LIST *l, SV_CHUNK *c);
int svAddScript2(SV_IMAGE_LIST *l, SV_CHUNK *c);
void svAnalyzeChunk(SV_IMAGE_LIST *l, SV_CHUNK **pp);
void svDeleteImageMapper(int idx);

/* ------------------------------------------------------------------ */

void svImageListDestroy(SV_IMAGE_LIST *p)
{
    int i;

    for (i = 0; i < 256; i++) {
        if (p->item[i].used != 0) {
            p->item[i].used = 0;
        }
    }
    p->num = 0;
    p->unk00 = 0;
    p->id = 0;
    p->data = 0;
    p->vramPtr = p->vramBase;
}

int svImageListAlloc(SV_IMAGE_LIST *p)
{
    int i;

    for (i = 0; i < 256; i++) {
        if (p->item[i].used == 0) {
            return i;
        }
    }
    return -1;
}

void *svGetTypeList(int idx) { return _imageMapper + idx * 0x241C; }

SV_IMAGE *svGetImageListItemSub(int idx, int key)
{
    SV_IMAGE_LIST *p = (SV_IMAGE_LIST *)svGetTypeList(idx);
    SV_IMAGE *it = p->item;
    unsigned char k = key;
    int i;

    for (i = 0; i < 256; i++) {
        if (it->used != 0) {
            if (*(unsigned char *)&it->attr == k) {
                return it;
            }
        }
        it++;
    }
    return 0;
}

SV_IMAGE *svGetImageListItem(int idx, int flags)
{
    int n;

    if (flags & 0x100) {
        n = 0;
    } else if (flags & 0x200) {
        n = idx;
    } else if (flags & 0x400) {
        n = idx;
    } else if (flags & 0x800) {
        n = 15;
    } else {
        n = (flags & 0x1000) ? idx : 14;
    }
    return svGetImageListItemSub(n, flags);
}

SV_IMAGE *svGetResFromName(char *name)
{
    SV_IMAGE_LIST *p = (SV_IMAGE_LIST *)svGetTypeList(0);
    SV_IMAGE *it = p->item;
    int i;

    for (i = 0; i < 256; i++) {
        if (it->used != 0) {
            if (strcmp(it->name, name) == 0) {
                return it;
            }
        }
        it++;
    }
    return 0;
}

void svLoadTexture(SV_IMAGE *p)
{
    int tbp = p->tbp;
    int h = p->h;
    int w = p->w;
    void *pkt;
    int tbw;

    pkt = xglPacketGetCurrent();
    if (p->bpp == 8) {
        tbw = p->tbw;
        sceVif1PkRefLoadImage(pkt, tbp, 19, tbw >> 6,
                              (unsigned int)p->data, tbw * h / 16, 0, 0,
                              w, h);
    } else {
        tbw = p->tbw;
        sceVif1PkRefLoadImage(pkt, tbp, 20, tbw >> 6,
                              (unsigned int)p->data, tbw * h / 16, 0, 0,
                              w, h);
    }
}

void svLoadClut(SV_IMAGE *p)
{
    void *pkt = xglPacketGetCurrent();
    int bpp = p->bpp;
    unsigned int tbp = p->tbp;

    if (bpp == 8) {
        sceVif1PkRefLoadImage(pkt, tbp, 0, 1, (unsigned int)p->data, 64,
                              0, 0, 16, 16);
    } else {
        sceVif1PkRefLoadImage(pkt, tbp, 0, 1, (unsigned int)p->data, 4,
                              0, 0, 8, 2);
    }
}

int svLoadImageList(SV_IMAGE_LIST *l)
{
    int i;
    int n = 0;

    for (i = 0; i < 256; i++) {
        if (l->item[i].used != 0) {
            if (l->item[i].kind == 4) {
                svLoadTexture(&l->item[i]);
                n++;
            } else if (l->item[i].kind == 3) {
                svLoadClut(&l->item[i]);
            }
        }
    }
    return n;
}

void svLoadMapperList(void)
{
    int i;
    int off;

    for (i = 0, off = 0; i < 40; i++, off += 0x241C) {
        if (svImageMapper[i].num > 0) {
            svLoadImageList((SV_IMAGE_LIST *)(_imageMapper + off));
        }
    }
}

int svGetSizeBit(int flags)
{
    int bit = 2;
    int n = 1;
    do {
        if ((flags & bit) != 0) {
            return n;
        }
        n++;
        bit <<= 1;
    } while (n < 32);
    return 0;
}

/* NEAR MISS (87 diffs, 88 orig vs 90 built): the original keeps `w` and
 * `h` in callee-saved registers across both svGetSizeBit calls ($s1 from
 * the andi of the just-stored p->w, $s3 from a reload of p->h); gcc here
 * rematerialises both from memory instead and so uses four callee-saved
 * registers where the original uses six. Everything else -- the psm == 4
 * split, both divisions and the vramPtr bump -- lines up. */
int svAddImage(SV_IMAGE_LIST *l, SV_CHUNK *c)
{
    SV_IMAGE *p;
    int idx = svImageListAlloc(l);
    unsigned int size;
    unsigned int ptr;

    if (idx < 0) {
        return -1;
    }
    p = &l->item[idx];
    p->kind = 4;
    p->used = 1;
    p->data = (char *)c + 64;
    p->attr = c->attr;
    p->w = c->w;
    p->h = c->h;
    p->bpp = c->bpp;
    p->wbit = svGetSizeBit(p->w);
    p->hbit = svGetSizeBit(p->h);
    p->name = c->name;
    size = c->dataSize;
    if (c->bpp == 4) {
        p->tbw = c->w >> 1;
        if (p->h * 2 < p->w) {
            size = size * (p->w / (p->h * 2));
        }
    } else if (p->w < p->h) {
        p->tbw = c->w * p->h / p->w;
    } else {
        p->tbw = c->w;
    }
    ptr = l->vramPtr;
    p->tbp = ptr >> 6;
    l->vramPtr = ptr + (size >> 2);
    return idx;
}

/* NEAR MISS (16 diffs, same length): instruction-for-instruction right,
 * but three values land in different registers -- the original puts the
 * payload address in $a0, the constant 1 in $v0 and the attr in $v1,
 * this build $a1/$v1/$a0 -- and the four tail stores schedule one slot
 * apart. All 24 orderings of the tail block were swept (tools/permute).
 * m2c agrees with the statement order used here. */
int svAddClut(SV_IMAGE_LIST *l, SV_CHUNK *c)
{
    SV_IMAGE *p;
    int idx = svImageListAlloc(l);
    unsigned int ptr;
    unsigned short bpp;
    char *data = (char *)c + 64;

    if (idx < 0) {
        return -1;
    }
    p = &l->item[idx];
    p->kind = 3;
    p->used = 1;
    p->data = data;
    p->attr = c->attr;
    p->w = c->w;
    p->h = c->h;
    bpp = c->bpp;
    p->bpp = bpp;
    p->wbit = svGetSizeBit(p->w);
    p->hbit = svGetSizeBit(p->h);
    ptr = l->vramPtr;
    l->vramPtr = ptr + (bpp == 8 ? 256 : 128);
    p->tbw = c->w * 4;
    p->name = c->name;
    p->tbp = ptr >> 6;
    return idx;
}

int svAddModel(SV_IMAGE_LIST *l, SV_CHUNK *c)
{
    SV_IMAGE *p;
    int idx = svImageListAlloc(l);
    char *data = (char *)c + 64;
    char *name = c->name;

    if (idx < 0) {
        return -1;
    }
    p = &l->item[idx];
    p->kind = 6;
    p->name = name;
    p->data = data;
    p->attr = c->attr | 0x2000;
    p->used = 1;
    p->h = 0;
    p->w = 0;
    return idx;
}

int svAddScript(SV_IMAGE_LIST *l, SV_CHUNK *c)
{
    SV_IMAGE *p;
    int idx = svImageListAlloc(l);
    char *data = (char *)c + 64;
    char *name = c->name;

    if (idx < 0) {
        return -1;
    }
    p = &l->item[idx];
    p->kind = 5;
    p->name = name;
    p->data = data;
    p->attr = c->attr;
    p->used = 1;
    p->h = 0;
    p->w = 0;
    return idx;
}

int svAddScript2(SV_IMAGE_LIST *l, SV_CHUNK *c)
{
    SV_IMAGE *p;
    int idx = svImageListAlloc(l);
    char *data = (char *)c + 192;
    char *name = c->name;

    if (idx < 0) {
        return -1;
    }
    p = &l->item[idx];
    p->kind = 5;
    p->name = name;
    p->data = data;
    p->attr = c->attr;
    p->used = 1;
    p->h = 0;
    p->w = 0;
    return idx;
}

void svAnalyzeChunk(SV_IMAGE_LIST *l, SV_CHUNK **pp)
{
    SV_CHUNK *c = *pp;

    while (c->type != 0) {
        switch (c->type) {
        case 1:
        case 2:
            c = (SV_CHUNK *)((char *)c + 64);
            continue;
        case 4:
            svAddImage(l, c);
            break;
        case 3:
            svAddClut(l, c);
            break;
        case 7:
            /* NEAR MISS: gcc cross-jumps this into the identical case 4
             * body and redirects the jump-table entry, so the built
             * function is one 5-word block (plus its alignment nop)
             * shorter than the original, which kept both. */
            svAddImage(l, c);
            break;
        case 6:
        case 9:
            svAddModel(l, c);
            break;
        case 5:
            svAddScript(l, c);
            break;
        case 10:
            svAddScript2(l, c);
            break;
        default:
            return;
        }
        c = (SV_CHUNK *)((char *)c + c->size);
    }
}

/* NEAR MISS (16 diffs, same length): the original's store induction
 * variable is biased to &list[i].id (offset +16), so the six stores use
 * offsets -12..+4; gcc bases it on the struct start and uses +4..+20.
 * All 720 orderings of the store block were swept, and a short* alias
 * for the three halfword stores does not move the base either. */
void svInitImageMapper(void)
{
    static unsigned int siz[40] __asm__("siz.0") = {
        0x6000, 0x2000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000,
        0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x10000, 0x20000,
        0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000,
        0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000,
        0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000,
        0x20000, 0x20000, 0x20000
    };
    static unsigned int top[40] __asm__("top.1") = {
        0xE0000, 0xE6000, 0xE8000, 0xED500, 0xF2A00, 0xEC000, 0xF1500,
        0xF6A00, 0xF0000, 0xF5500, 0xFAA00, 0xF4000, 0xF8000, 0xFC000,
        0xE8000, 0xE8000, 0xE8000, 0xE8000, 0xE8000, 0xE8000, 0xE8000,
        0xE8000, 0xE8000, 0xE8000, 0xE8000, 0xE8000, 0xE8000, 0xE8000,
        0xE8000, 0xE8000, 0xE8000, 0xE8000, 0xE8000, 0xE8000, 0xE8000,
        0xE8000, 0xE8000, 0xE8000, 0xE8000, 0xE8000
    };
    char *m = _imageMapper;
    SV_IMAGE_LIST *p = (SV_IMAGE_LIST *)m;
    int i;

    memset(m, 0, 40 * sizeof(SV_IMAGE_LIST) + 40 * sizeof(SV_REF));
    for (i = 0; i < 40; i++) {
        p[i].vramBase = top[i];
        p[i].vramSize = siz[i];
        p[i].vramPtr = top[i];
        p[i].id = 0;
        p[i].data = 0;
        p[i].num = 0;
    }
}

/* NEAR MISS (2 diffs): same shape, but the original biases the store
 * induction variable to &ref[i].unk04, giving `sw -4(v0)` / `sw 0(v0)`
 * off base+4 where this build uses base+0 and `sw 0` / `sw 4`. The
 * lui/ori constant differs by exactly that 4. Everything else -- including
 * forming the offset at run time rather than folding it into %hi/%lo,
 * which is why the base has to be a pointer variable -- matches. */
void svInitRefImage(void)
{
    char *m = _imageMapper;
    SV_REF *p = (SV_REF *)(m + 40 * sizeof(SV_IMAGE_LIST));
    int i;

    for (i = 0; i < 40; i++) {
        p[i].unk04 = 0;
        p[i].unk00 = 0;
    }
}

void svAddImageMapper(int idx, int id, SV_CHUNK **pp, int data)
{
    SV_IMAGE_LIST *p = (SV_IMAGE_LIST *)svGetTypeList(idx);

    if (p->num != 0) {
        svImageListDestroy(p);
    }
    p->data = data;
    p->id = id;
    p->num++;
    svAnalyzeChunk(p, pp);
}

/* NEAR MISS (15 diffs, same length): gcc eliminates the loop counter and
 * compares the field pointer against base + 14*0x241C; the original keeps
 * `i` and tests `slti 14`, which also lets it fold the +16 field offset
 * into the induction variable's initial value. The explicit byte-offset
 * variable that fixes svLoadMapperList is not enough here -- the early
 * return gives the loop a second exit, so check_dbra_loop cannot fire and
 * gcc falls back to biv elimination. */
void svDeleteImageMapperID(int id)
{
    int i;
    int off;

    for (i = 2, off = 2 * 0x241C; i < 14; i++, off += 0x241C) {
        if (svImageMapper[i].id == id) {
            svImageListDestroy((SV_IMAGE_LIST *)(_imageMapper + off));
            return;
        }
    }
}

void svDeleteImageMapperData(int data)
{
    int i;
    int off;

    for (i = 2, off = 2 * 0x241C; i < 14; i++, off += 0x241C) {
        if (svImageMapper[i].data == data) {
            svImageListDestroy((SV_IMAGE_LIST *)(_imageMapper + off));
            return;
        }
    }
}

void *svGetImageItem(int idx) { return &_imageList.item[idx]; }

extern int _draw3D;
extern int _nEffect2D;
extern int _nowImage;
extern void svDrawSchedulerParticle(void);
void svDrawScheduler(void)
{
    _draw3D = 0;
    _nEffect2D = 0;
    _nowImage = 0;
    svDrawSchedulerParticle();
}

void svImageListCreate(void) { memset(&_imageList, 0, sizeof(SV_IMAGE_LIST)); }

extern void sdvDrawAlters(void);
void svDrawAlters(int cate)
{
    if (_draw3D == 0) {
        sdvDrawAlters();
    }
}

extern void svDrawAlters(int cate);
extern int _scMslCate;
extern void MEfObjExec2nd(int a);
/* PIN to $a0: gcc otherwise copies the parameter into $v1 for the compare
 * and leaves $a0 for the tail call; the original compares against $a0
 * directly. The pin sticks because n is also the call argument. */
void svDrawMissile(int a)
{
    PIN(int n, "$4");

    n = a;
    if (_draw3D == 0) {
        if (_scMslCate == n) {
            MEfObjExec2nd(n);
        }
    }
}

void svDrawScheduler2D(void)
{
    if (_nEffect2D != 0) {
        _nowImage = 0;
        _draw3D = 1;
        svDrawSchedulerParticle();
    }
}

void svDeleteImageMapper(int idx)
{
    SV_IMAGE_LIST *p = (SV_IMAGE_LIST *)svGetTypeList(idx);
    if (p->num != 0) {
        svImageListDestroy(p);
    }
}

/* ------------------------------------------------------------------ */
/* Per-frame draw scheduling.                                          */

/* One effect scheduler slot, 0xAB0 bytes; sefGetScheduler returns the
 * base of a 128-entry array of them. */
typedef struct {
    /* 0x000 */ char pad000[0x6B0];
    /* 0x6B0 */ int used;
    /* 0x6B4 */ char pad6B4[0xA78 - 0x6B4];
    /* 0xA78 */ short eftNo;
    /* 0xA7A */ char padA7A[0xA8C - 0xA7A];
    /* 0xA8C */ int flags;
    /* 0xA90 */ char padA90[0xAB0 - 0xA90];
} SV_SCHED;

/* Only the parts of the srs resource block this file touches. */
typedef struct {
    /* 0x000 */ char pad000[0x38];
    /* 0x038 */ void *buf;              /* the 256K effect-script arena */
    /* 0x03C */ char pad03C[0xBC - 0x3C];
    /* 0x0BC */ short loaded;
    /* 0x0BE */ char pad0BE[0x10C - 0xBE];
    /* 0x10C */ short eftNo[26];        /* indexed by mapper slot - 14 */
} SV_MEMRES;

extern SV_MEMRES _srsMemRes;
extern SV_SCHED *sefGetScheduler(void);
extern void srsAnalyzeEftNo(int no, int *charID, int *cate);
extern void SGsSetZTestEnv(int on);
extern void sefDrawSchedulerEffect(int idx);
extern void SGsInitEnv(void);
extern void SGsRestoreEnv(void);
extern void MEfObjExec1st(void);
extern void svInitRefImage(void);
void svDrawSchedulerBlk(int cate, int no);

/* Draw every live scheduler slot whose effect belongs to one category.
 * `no` of 0 means "any effect", otherwise only the named one. */
void svDrawSchedulerBlk(int cate, int no)
{
    static int charID __asm__("charID.3");
    static int eftCate __asm__("eftCate.4");
    static int eft __asm__("eft.5");
    SV_SCHED *s;
    /* Allocator tie-break: gcc gives the loop counter $s1 and the `no`
     * parameter $s2; the original build chose the other way round. The
     * pin sticks because i is also the sefDrawSchedulerEffect argument. */
    PIN(int i, "$18");

    s = sefGetScheduler();
    for (i = 0; i < 128; i++) {
        if (s->used != 0) {
            if (eft != s->eftNo) {
                eft = s->eftNo;
                srsAnalyzeEftNo(eft, &charID, &eftCate);
            }
            if (eftCate == cate) {
                if (no == 0 || (no > 0 && no == eft)) {
                    SGsSetZTestEnv(((s->flags >> 2) ^ 1) & 1);
                    sefDrawSchedulerEffect(i);
                }
            }
        }
        s++;
    }
}

/* The 3D pass: one scheduler sweep per image-mapper category. */
void svDrawScheduler3D(void)
{
    int mode = 2;
    int i;
    SV_IMAGE_LIST *p;

    _draw3D = mode;
    svInitRefImage();
    SGsInitEnv();
    p = (SV_IMAGE_LIST *)_imageMapper;
    if (p[0].num > 0) {
        _nowImage = 0;
        svDrawSchedulerBlk(0, 0);
    }
    _nowImage = mode;
    svDrawSchedulerBlk(2, 0);
    _nowImage = 11;
    svDrawSchedulerBlk(11, 0);
    _nowImage = 14;
    svDrawSchedulerBlk(14, 0);
    _nowImage = 15;
    svDrawSchedulerBlk(15, 0);
    for (i = 16; i < 40; i++) {
        if (p[i].num > 0) {
            if (_srsMemRes.eftNo[i - 14] > 0) {
                _nowImage = i;
                svDrawSchedulerBlk(16, _srsMemRes.eftNo[i - 14]);
            }
        }
    }
    MEfObjExec1st();
    SGsRestoreEnv();
}

/* ------------------------------------------------------------------ */
/* Effect script lookup and loading.                                   */

extern int sprintf(char *buf, const char *fmt, ...);
extern void *srsGetEsdData(int no);
extern int sefSearchMapperIndex(int no);
extern int srsFileLoadCf(void *p, int no);
extern int sefIsFinishEffect2(void);
extern int *sefGetLoadQue(void);
extern void sefSetLoadQue(int v);
extern void *smAlloc(unsigned int size);
extern int srsLoadEffectData(void *buf, int no);

/* The texture parameters one named image expands to: a ready-made GS
 * TEX0 register plus the size in the game's 1/100 units. */
typedef struct {
    /* 0x00 */ unsigned long tex0;
    /* 0x08 */ float w;
    /* 0x0C */ float h;
} SV_PRM;

/* Look "<name>.bmp" and "<name>.clt" up in type list 0 and build the
 * TEX0 register that draws the pair. */
/* NEAR MISS (48 diffs, 79 orig vs 81 built): the TEX0 field packing and
 * the block layout match; the original keeps the clut pointer in $v0
 * across the null test (`beqz v0`) and sets the return value early,
 * while gcc copies it to $t0 first, costing one instruction. */
SV_PRM *svGetPrmFromName(char *name, SV_PRM *prm)
{
    char buf[128];
    SV_IMAGE *img;
    SV_IMAGE *clut;
    int psm;

    sprintf(buf, "%s.bmp", name);
    img = svGetResFromName(buf);
    sprintf(buf, "%s.clt", name);
    clut = svGetResFromName(buf);
    if (img != 0 && clut != 0) {
        prm->h = img->h * 0.01f;
        prm->w = img->w * 0.01f;
        psm = img->bpp == 8 ? 19 : 20;
        prm->tex0 = (unsigned long)img->tbp
                  | ((unsigned long)psm << 20)
                  | (((unsigned long)img->tbw >> 6) << 14)
                  | ((unsigned long)img->wbit << 26)
                  | ((unsigned long)img->hbit << 30)
                  | ((unsigned long)1 << 34)
                  | ((unsigned long)clut->tbp << 37)
                  | ((unsigned long)1 << 61);
        return prm;
    }
    memset(prm, 0, 16);
    return 0;
}

/* Find the script slot whose name prefixes the effect's script name. */
SV_IMAGE *svGetScript(int no)
{
    char *name = (char *)srsGetEsdData(no);
    SV_IMAGE_LIST *p;
    int idx;
    int i;

    if (name == 0) {
        return 0;
    }
    idx = sefSearchMapperIndex(no);
    p = (SV_IMAGE_LIST *)(_imageMapper + idx * 0x241C);
    if (idx == 14) {
        if (p->item[0].used != 0 && p->item[0].kind != 0) {
            return &p->item[0];
        }
    }
    for (i = 0; i < 256; i++) {
        if (p->item[i].used != 0 && p->item[i].kind == 5
            && p->item[i].name != (char *)-1) {
            if (strncmp(name, p->item[i].name, strlen(name)) == 0) {
                return &p->item[i];
            }
        }
    }
    return 0;
}

/* Bring one effect's script resource in, reusing the shared arena. */
/* NEAR MISS (47 diffs, same length): the original allocates `no` to $s0
 * and `p` to $s1, which lets $s1 be reused for the _srsMemRes address
 * once `p` dies after the cate == 16 branch; gcc allocates them the other
 * way round and needs a third callee-saved register. Pinning `no` only
 * adds a register rather than swapping the pair. */
int svFileLoadScript(void *p, int no)
{
    int charID;
    int cate;
    void *buf;

    srsAnalyzeEftNo(no, &charID, &cate);
    if (cate == 16) {
        return srsFileLoadCf(p, no);
    }
    if (cate != 14) {
        return -1;
    }
    if (_srsMemRes.buf != 0) {
        if (_srsMemRes.eftNo[0] == no) {
            return 14;
        }
        if (sefIsFinishEffect2() == 0) {
            sefGetLoadQue();
            sefSetLoadQue(no);
            return -1;
        }
        svDeleteImageMapper(14);
    }
    sefSetLoadQue(0);
    buf = _srsMemRes.buf;
    if (buf == 0) {
        buf = smAlloc(0x40000);
        _srsMemRes.buf = buf;
        if (buf == 0) {
            return -1;
        }
    }
    _srsMemRes.eftNo[0] = no;
    _srsMemRes.loaded = 1;
    if (srsLoadEffectData(buf, no) < 0) {
        _srsMemRes.eftNo[0] = 0;
        _srsMemRes.loaded = 0;
        return -1;
    }
    return 14;
}

/* ------------------------------------------------------------------ */

extern void SGsTexFlush(void);

/* The 2D/particle pass. Each mapper category is loaded into GS memory
 * on demand and then drawn; in the _draw3D == 1 sub-pass a category is
 * skipped unless its reference-image slot says something wants it. */
/* NEAR MISS (151 diffs, 209 orig vs 201 built): the control flow and all
 * five category blocks line up. The residue is addressing: the original
 * CSEs %hi(_imageMapper) into $s5 and re-adds %lo per use, so every
 * reference-table access is (register + run-time constant); gcc here
 * folds each reference offset into its own %hi/%lo pair, one instruction
 * shorter per site. Basing them off the `m` pointer gets the run-time add
 * but then gcc hoists the whole base into a callee-saved register. */
void svDrawSchedulerParticle(void)
{
    char *m = _imageMapper;
    int draw3D = _draw3D;
    int n;
    int i;
    int off;

    SGsInitEnv();
    if (svImageMapper[0].num > 0) {
        if (svLoadImageList(&svImageMapper[0]) > 0) {
            SGsTexFlush();
        }
        svDrawSchedulerBlk(0, 0);
        svDrawMissile(0);
    }
    if (draw3D != 1 || SV_REFP(m)[14].unk04 > 0 || SV_REFP(m)[2].unk04 > 0) {
        n = 0;
        for (i = 2, off = 2 * 0x241C; i < 11; i++, off += 0x241C) {
            if (svImageMapper[i].num > 0) {
                n += svLoadImageList((SV_IMAGE_LIST *)(_imageMapper + off));
            }
        }
        if (n > 0) {
            SGsTexFlush();
        }
        svDrawSchedulerBlk(2, 0);
        svDrawAlters(2);
        svDrawMissile(2);
    }
    if (draw3D != 1 || SV_REFP(m)[14].unk04 > 0 || SV_REFP(m)[11].unk04 > 0) {
        n = 0;
        for (i = 11, off = 11 * 0x241C; i < 14; i++, off += 0x241C) {
            if (svImageMapper[i].num > 0) {
                n += svLoadImageList((SV_IMAGE_LIST *)(_imageMapper + off));
            }
        }
        if (n > 0) {
            SGsTexFlush();
        }
        svDrawSchedulerBlk(11, 0);
        svDrawAlters(11);
        svDrawMissile(11);
    }
    if (draw3D != 1 || SV_REFP(m)[14].unk04 > 0) {
        if (svImageMapper[14].num > 0) {
            if (svLoadImageList(&svImageMapper[14]) > 0) {
                SGsTexFlush();
            }
            svDrawSchedulerBlk(14, 0);
            svDrawAlters(14);
            svDrawMissile(14);
        }
    }
    if (svImageMapper[15].num > 0) {
        if (svLoadImageList(&svImageMapper[15]) > 0) {
            SGsTexFlush();
        }
        svDrawSchedulerBlk(15, 0);
    }
    for (i = 16, off = 16 * 0x241C; i < 40; i++, off += 0x241C) {
        if (svImageMapper[i].num > 0) {
            if (draw3D != 1 || SV_REFP(m)[i].unk04 > 0) {
                if (svLoadImageList((SV_IMAGE_LIST *)(_imageMapper + off)) > 0) {
                    SGsTexFlush();
                }
                if (_srsMemRes.eftNo[i - 14] > 0) {
                    svDrawSchedulerBlk(16, _srsMemRes.eftNo[i - 14]);
                }
            }
        }
    }
    SGsRestoreEnv();
}
