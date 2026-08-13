/* Battle scene visual-effect scheduler functions (sv* family) */

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
