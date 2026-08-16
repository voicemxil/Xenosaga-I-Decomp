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
    int i;

    memset(_imageMapper, 0,
           40 * sizeof(SV_IMAGE_LIST) + 40 * sizeof(SV_REF));
    for (i = 0; i < 40; i++) {
        /* PERM_BEGIN */
        svImageMapper[i].vramBase = top[i];
        svImageMapper[i].vramSize = siz[i];
        svImageMapper[i].vramPtr = top[i];
        svImageMapper[i].id = 0;
        svImageMapper[i].data = 0;
        svImageMapper[i].num = 0;
        /* PERM_END */
    }
}

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
void svDrawAlters(void)
{
    if (_draw3D == 0) {
        sdvDrawAlters();
    }
}

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
