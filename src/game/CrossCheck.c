/* Cross-check predicates: line-of-sight tests between actors and map units */

typedef struct {
    float x;
    float y;
    float z;
    float w;
} VECTOR;

typedef struct {
    int nFlags;             /* 0x000 */
    char pad04[0x0C];       /* to 0x010 */
    VECTOR pos;             /* 0x010 */
    char pad20[0x60];       /* to 0x080 */
    unsigned char nIdx80;    /* 0x080 */
    char pad81[5];           /* to 0x086 */
    short nUnk86;             /* 0x086 */
    char pad88[0x18];        /* to 0x0A0 */
    unsigned char nTeam;      /* 0x0A0 */
    char padA1[0x82F];       /* to 0x8D0 */
    int nUnk8D0;              /* 0x8D0 */
    char pad8D4[0x114];      /* to 0x9E8 */
    float f9E8;               /* 0x9E8 */
    char pad9EC[0x84];       /* to 0xA70 */
} ACTOR_CC;

extern ACTOR_CC actor[];

/* The collision-shape sub-object every map unit carries at +0x1A0. The
   original code reaches its fields through a pointer to the sub-struct
   rather than through the unit, which is visible in the machine code all
   over this family: CrossCheckMapUnitAt does `addiu v1,v1,416` then
   `lb v1,45(v1)`, and CheckBrokenMapUnit does `addiu a1,a0,416` then
   `lb a0,4(a1)` / `lw v0,68(a1)`. */
typedef struct {
    char pad00[0x2D];        /* to 0x02D */
    signed char nType;        /* 0x02D  -- 1 = circle, 2 = box */
} MAPUNIT_SHAPE;

typedef struct {
    int nFlags;              /* 0x000 */
    char pad04[0x10];        /* to 0x014 */
    float fY;                 /* 0x014 */
    char pad18[0x88];        /* to 0x0A0 */
    unsigned char nTeam;      /* 0x0A0 */
    char padA1[3];           /* to 0x0A4 */
    short nUnkA4;              /* 0x0A4 */
    char padA6[0xFA];        /* to 0x1A0 */
    MAPUNIT_SHAPE shape;     /* 0x1A0 */
    char pad1CE[0x132];      /* to 0x300 */
} MAPUNIT_CC;

extern MAPUNIT_CC MapUnit[];

extern float fabsf(float f);
extern int CheckCrossCircle(void *a0, void *a1, void *a2, float radius);
extern int CrossCheckMapUnitCircle(VECTOR *pPos, void *pOther, MAPUNIT_CC *pUnit);
extern int CrossCheckMapUnitBox(VECTOR *pPos, void *pOther, MAPUNIT_CC *pUnit);

/* True when a target point sits close enough to a map unit's Y level and the
   unit's shape check (circle or box) reports a crossing hit */
int CrossCheckMapUnitAt(VECTOR *pPos, void *pOther, MAPUNIT_CC *pUnit)
{
    MAPUNIT_CC *m = pUnit;
    MAPUNIT_SHAPE *s = &m->shape;

    if (fabsf(pPos->y - m->fY) > 0.6f) {
        return 0;
    }
    switch (s->nType) {
    case 1:
        return CrossCheckMapUnitCircle(pPos, pOther, m);
    case 2:
        return CrossCheckMapUnitBox(pPos, pOther, m);
    }
    return 0;
}

/* Scan every map unit for one whose Y band, flags and shape make it cross
   the pPos/pOther segment; returns the matching index or -1.

   The shape pointer has to be formed BEFORE the unit pointer: gcc's loop
   strength reduction anchors the induction variable on whichever address
   expression it sees first, so `s` first makes it materialise
   %hi/%lo(MapUnit+0x1A0) and reach the unit base with a second
   `addiu s0,v0,-416`, which is exactly what the original does. Deriving
   `s` from `m` instead folds the +0x1A0 away and loses that addiu. */
int CrossCheckMapUnit(VECTOR *pPos, void *pOther)
{
    int i;
    MAPUNIT_CC *m;
    MAPUNIT_SHAPE *s;

    for (i = 0; i < 64; i++) {
        s = &MapUnit[i].shape;
        m = &MapUnit[i];
        if (m->nUnkA4 == -1) {
            continue;
        }
        if ((m->nFlags & 0x100000) != 0) {
            continue;
        }
        if (s->nType == 0) {
            continue;
        }
        if ((m->nFlags & 0x10000) == 0) {
            continue;
        }
        if (CrossCheckMapUnitAt(pPos, pOther, m) != 0) {
            return i;
        }
    }
    return -1;
}

/* Scan every actor for one (other than pRef itself) within 0.3 Y units of
   pRef whose circle crosses the pRef/arg segment; returns the matching
   index or -1.

   Everything here indexes actor[i] directly rather than through a cached
   pointer. That is what splits the third CheckCrossCircle argument off into
   its own induction variable: the field reads become address-form givs that
   share one register, while `&actor[i].pos` is a value-form giv, which
   gcc 2.9x materialises as (actor+0x10) hoisted out of the loop plus a
   separate i*0xA70 accumulator. A cached `pA` pointer folds that into one
   `addiu a2,pA,16` and loses seven instructions. */
int CrossCheckActor(ACTOR_CC *pRef, void *arg)
{
    int i;

    for (i = 0; i < 64; i++) {
        if (fabsf(pRef->pos.y - actor[i].pos.y) > 0.3f) {
            continue;
        }
        if (actor[i].nUnk86 < 0) {
            continue;
        }
        if ((actor[i].nFlags & 8) != 0) {
            continue;
        }
        if (actor[i].nUnk8D0 == 0) {
            continue;
        }
        if (i == pRef->nIdx80) {
            continue;
        }
        if (CheckCrossCircle(&pRef->pos, arg, &actor[i].pos, actor[i].f9E8)) {
            return i;
        }
    }
    return -1;
}

extern int CrossCheckMapUnitAt(VECTOR *pPos, void *pOther, MAPUNIT_CC *pUnit);

/* Line-of-sight scan for an aimed shot: like CrossCheckMapUnit, but it also
   skips the aiming actor's own team and units flagged 0x7000.

   The original loop carries both a pointer into the current unit and a byte
   offset from MapUnit.  Its pointer is initialized through the shape member
   and then backed up to nTeam; that is why the prologue materializes
   MapUnit+0x1A0 before subtracting 0x100.  Forming `unit` after the two
   nUnkA4 tests also lets the compiler keep it in the third argument register
   while the remaining predicates run. */
int CrossCheckMapUnitAim(VECTOR *pPos, ACTOR_CC *pRef)
{
    int i;
    int offset;
    MAPUNIT_SHAPE *shape;
    MAPUNIT_CC *unit;
    unsigned char *team;

    shape = &MapUnit[0].shape;
    team = (unsigned char *)shape - 0x100;
    offset = 0;
    for (i = 0; i < 64; i++, team += sizeof(MAPUNIT_CC), offset += sizeof(MAPUNIT_CC)) {
        if (*(short *)(team + 4) == -1) {
            continue;
        }
        if (*(short *)(team + 4) == 0x7000) {
            continue;
        }
        unit = (MAPUNIT_CC *)((char *)MapUnit + offset);
        if ((*(int *)(team - 0xA0) & 0x100000) != 0) {
            continue;
        }
        if (*team == pRef->nTeam) {
            continue;
        }
        if (*(signed char *)(team + 0x12D) == 0) {
            continue;
        }
        if ((*(int *)(team - 0xA0) & 0x10000) == 0) {
            continue;
        }
        if (CrossCheckMapUnitAt(pPos, &pRef->pos, unit) != 0) {
            return i;
        }
    }
    return -1;
}
