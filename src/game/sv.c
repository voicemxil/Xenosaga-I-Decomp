/* Battle scene visual-effect scheduler functions (sv* family) */

#include "matching.h"

extern char _imageMapper[];
void *svGetTypeList(int idx) { return _imageMapper + idx * 0x241C; }

extern char D_0040D69C[];
void *svGetImageItem(int idx) { return D_0040D69C + idx * 36; }

extern int _draw3D;
extern int _nEffect2D;
extern int _nowImage;
extern void svDrawSchedulerParticle(void);
void svDrawScheduler(void) {
    _draw3D = 0;
    _nEffect2D = 0;
    _nowImage = 0;
    svDrawSchedulerParticle();
}

extern int _imageList[];
extern void *memset(void *, int, unsigned int);
void svImageListCreate(void) { memset(_imageList, 0, 0x241C); }

extern void sdvDrawAlters(void);
void svDrawAlters(void) {
    if (_draw3D == 0) {
        sdvDrawAlters();
    }
}

extern int _scMslCate;
extern void MEfObjExec2nd(int a);
/* PIN to $a0: gcc otherwise copies the parameter into $v1 for the compare
 * and leaves $a0 for the tail call; the original compares against $a0
 * directly. The pin sticks because n is also the call argument. */
void svDrawMissile(int a) {
    PIN(int n, "$4");

    n = a;
    if (_draw3D == 0) {
        if (_scMslCate == n) {
            MEfObjExec2nd(n);
        }
    }
}

extern void svDrawSchedulerParticle(void);
void svDrawScheduler2D(void) {
    if (_nEffect2D != 0) {
        _nowImage = 0;
        _draw3D = 1;
        svDrawSchedulerParticle();
    }
}

typedef struct { char pad[0x14]; short flag; } SV_MAPPER_HDR;
extern void svImageListDestroy(void *p);
void svDeleteImageMapper(int idx) {
    SV_MAPPER_HDR *p = (SV_MAPPER_HDR *)svGetTypeList(idx);
    if (p->flag != 0) {
        svImageListDestroy(p);
    }
}

int svGetSizeBit(int flags) {
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

