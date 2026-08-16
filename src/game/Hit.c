/* Collision-query helpers used by map-unit and actor game logic. */

#include "matching.h"

extern int HitCheckMapUnitPosSize(void *unit, void *position, float size);
extern int HitCheckMapUnitAt(void *position, void *unit);
extern int HitCheckMapUnitPosAt(void *position, void *unit);
extern float CheckDist2D(void *a, void *b);
extern int HitCheckBox(void *position, void *unit);
extern int HitCheckBoxCorner(void *position, void *unit);
extern char D_0047AEC0[];

typedef struct HitVector {
    float x;
    float y;
    float z;
    float w;
} HitVector;

typedef struct HitProbe {
    char pad_000[0x10];
    HitVector position;
    char pad_020[0x80];
    unsigned char team;
    char pad_0A1[0x947];
    float size;
    char pad_9EC[0x80];
} HitProbe;

typedef struct HitBounds {
    float radius;
    float height;
    float corner_radius;
    float unknown;
} HitBounds;

typedef union HitAlignedVector {
    HitVector value;
    long long alignment[2];
} HitAlignedVector;

typedef union HitAlignedBounds {
    HitBounds value;
    long long alignment[2];
} HitAlignedBounds;

/* The collision-shape sub-object a map unit carries at +0x1A0. The original
   code walks and tests it through its own pointer rather than through the
   unit -- that is where the extra `addiu vX,unit,416` in this family comes
   from, and it is the same sub-struct CrossCheck.c names `shape`. */
typedef struct HitMapUnitShape {
    char pad_000[4];
    char container;         /* unit + 0x1A4 */
    char pad_005[0x28];
    char active;            /* unit + 0x1CD */
} HitMapUnitShape;

typedef struct HitMapUnit {
    unsigned int status;
    char pad_004[0x0C];
    HitAlignedVector position;
    char pad_020[0x60];
    HitVector *direction;
    char pad_084[0x20];
    short index;
    char pad_0A6[0x0A];
    HitAlignedBounds bounds;
    char pad_0C0[0xE0];
    HitMapUnitShape shape;  /* 0x1A0 */
    char pad_1CE[0x132];
} HitMapUnit __attribute__((aligned(8)));

extern HitMapUnit MapUnit[];

typedef struct HitUwamonoView {
    unsigned char team;
    char pad_001[3];
    short index;
    char pad_006[0x127];
    char active;
    char pad_12E[0x1D2];
} HitUwamonoView;

extern int HitCheckUwamonoAt(void *position, void *unit);

/* Test a point against a map unit with no radius expansion */
int HitCheckMapUnitPos(void *unit, void *position)
{
    return HitCheckMapUnitPosSize(unit, position, 0.0f);
}

/* Find the first collidable map unit which contains the position. */
int HitCheckMapUnit(void *position)
{
    HitMapUnit *unit = (HitMapUnit *)(D_0047AEC0 - 0x1A0);
    int i;

    for (i = 0; i < 0x40; i++, unit++) {
        if (unit->index == -1) {
            continue;
        }
        if (unit->status & 0x100000) {
            continue;
        }
        if (unit->shape.active == 0) {
            continue;
        }
        if ((unit->status & 0x10000) == 0) {
            continue;
        }
        if (HitCheckMapUnitAt(position, unit) != 0) {
            return i;
        }
    }
    return -1;
}

/* Find the last collidable map unit which contains the position. */
int HitCheckMapUnitReverse(void *position)
{
    HitMapUnit *unit = (HitMapUnit *)(D_0047AEC0 - 0x1A0);
    int i;

    unit += 0x40;
    for (i = 0x40; i > 0; i--, unit--) {
        if (unit->index == -1) {
            continue;
        }
        if (unit->status & 0x100000) {
            continue;
        }
        if (unit->shape.active == 0) {
            continue;
        }
        if ((unit->status & 0x10000) == 0) {
            continue;
        }
        if (HitCheckMapUnitAt(position, unit) != 0) {
            return i;
        }
    }
    return -1;
}

/* Find the first map unit containing a copied position and radius. */
int HitCheckMapUnitPosSize(void *position, void *unused, float size)
{
    HitProbe probe;
    HitMapUnit *unit = (HitMapUnit *)(D_0047AEC0 - 0x1A0);
    int i;

    (void)unused;
    probe.position = *(HitVector *)position;
    probe.size = size;
    for (i = 0; i < 0x40; i++, unit++) {
        if (unit->index == -1) {
            continue;
        }
        if (unit->shape.active == 0) {
            continue;
        }
        if ((unit->status & 0x10000) == 0) {
            continue;
        }
        if (HitCheckMapUnitPosAt(&probe, unit) != 0) {
            return i;
        }
    }
    return -1;
}

/* Find the last map unit containing a copied position. */
int HitCheckMapUnitPosReverse(void *position)
{
    HitProbe probe;
    HitMapUnit *unit = (HitMapUnit *)(D_0047AEC0 - 0x1A0);
    int i;

    probe.position = *(HitVector *)position;
    probe.size = 0.0f;
    unit += 0x40;
    for (i = 0x40; i > 0; i--, unit--) {
        if (unit->index == -1) {
            continue;
        }
        if (unit->shape.active == 0) {
            continue;
        }
        if ((unit->status & 0x10000) == 0) {
            continue;
        }
        if (HitCheckMapUnitAt(&probe, unit) != 0) {
            return i;
        }
    }
    return -1;
}

/* Find a collidable map unit outside one excluded container slot.

   The shape pointer is the loop's first address expression, so gcc anchors
   the induction variable on MapUnit+0x1A0 and reaches the unit fields at
   negative offsets from it; `&MapUnit[i]` is only ever a value (the call
   argument), so it becomes its own accumulator plus a hoisted base. */
int HitCheckContainerPosSize(void *position, int excluded, float size)
{
    HitProbe probe;
    HitMapUnitShape *shape;
    int i;

    probe.position = *(HitVector *)position;
    probe.size = size;
    for (i = 0; i < 0x40; i++) {
        shape = &MapUnit[i].shape;
        if (MapUnit[i].index == -1) {
            continue;
        }
        if (shape->container != 0) {
            continue;
        }
        if (i == excluded) {
            continue;
        }
        if (shape->active == 0) {
            continue;
        }
        if ((MapUnit[i].status & 0x10000) == 0) {
            continue;
        }
        if (HitCheckMapUnitPosAt(&probe, &MapUnit[i]) != 0) {
            return i;
        }
    }
    return -1;
}

/* Find a collidable map unit belonging to another team. */
int HitCheckUwamono(void *position)
{
    HitUwamonoView *view = (HitUwamonoView *)(D_0047AEC0 - 0x100);
    int i = 0;
    int offset = 0;
    HitMapUnit *base = (HitMapUnit *)(D_0047AEC0 - 0x1A0);
    HitMapUnit *unit;
    unsigned int status;

    while (i < 0x40) {
        if (view->index == -1) {
            goto next;
        }
        if (view->team == ((HitProbe *)position)->team) {
            goto next;
        }
        unit = (HitMapUnit *)((unsigned int)offset + (unsigned int)base);
        status = *(unsigned int *)((char *)view - 0xA0);
        if (status & 0x20000) {
            goto next;
        }
        if (status & 0x100000) {
            goto next;
        }
        if (view->active == 0) {
            goto next;
        }
        if ((status & 0x10000) == 0) {
            goto next;
        }
        if (HitCheckUwamonoAt(position, unit) != 0) {
            return i;
        }
next:
        i++;
        view++;
        offset += 0x300;
    }
    return -1;
}

/* Test a probe against one map unit's vertical span and collision shape. */
/* TODO: LENGTH near-match: the original retains an initial zero in $v0 and the
 * resulting case-label alignment nop; gcc removes both from this source shape
 * (53 words versus 55), while the collision branches and calls are equivalent. */
int HitCheckMapUnitAt(void *position, void *map_unit)
{
    HitProbe *probe = (HitProbe *)position;
    HitMapUnit *unit = (HitMapUnit *)map_unit;
    HitMapUnitShape *shape = &unit->shape;
    int result;
    float distance;

    if (unit->position.value.y + unit->bounds.value.height * 0.9f <
        probe->position.y) {
        result = 0;
    } else if (probe->position.y < unit->position.value.y - 1.8f) {
        result = 0;
    } else {
        switch (shape->active) {
        case 1:
            distance = CheckDist2D(&probe->position, &unit->position.value);
            if (distance < probe->size + unit->bounds.value.radius) {
                result = 1;
            } else {
                result = 0;
            }
            break;
        case 2:
            return HitCheckBox(probe, unit);
        default:
            result = 0;
            break;
        }
    }
    return result;
}

/* Test a probe against one map unit, including corner-box collision. */
/* TODO: LENGTH near-match: as in HitCheckMapUnitAt, gcc removes the original's
 * initial $v0 zero and case-label alignment nop (53 words versus 55). */
int HitCheckMapUnitPosAt(void *position, void *map_unit)
{
    HitProbe *probe = (HitProbe *)position;
    HitMapUnit *unit = (HitMapUnit *)map_unit;
    HitMapUnitShape *shape = &unit->shape;
    int result;
    float distance;

    if (unit->position.value.y + unit->bounds.value.height * 0.9f <
        probe->position.y) {
        result = 0;
    } else if (probe->position.y < unit->position.value.y - 1.8f) {
        result = 0;
    } else {
        switch (shape->active) {
        case 1:
            distance = CheckDist2D(&probe->position, &unit->position.value);
            if (distance < probe->size + unit->bounds.value.radius) {
                result = 1;
            } else {
                result = 0;
            }
            break;
        case 2:
            return HitCheckBoxCorner(probe, unit);
        default:
            result = 0;
            break;
        }
    }
    return result;
}

/* Test a circular probe against one oriented map-unit box. */
/* TODO: near-miss, 23 of 51 words (was 29). Length is exact and every
 * arithmetic step is the original's; what differs is the FP register
 * numbering and, feeding it, two scheduling choices: the original loads
 * the high half of the position copy first (ours loads the low half
 * first, and the two loads are independent so the scheduler is free), and
 * it interleaves the unit_position reload with the two subtractions
 * instead of doing both reloads up front. The declaration order above is
 * the best of an exhaustive 7! sweep of the initialised locals (5040
 * orderings, 29 -> 23); bounds must stay first or it loses its sp+0 stack
 * slot. What is left is not reachable by reordering these declarations --
 * it needs whatever makes gcc emit the 16-byte union copy high-word-first,
 * which the sweep never produced for the position copy while producing it
 * for the bounds copy in the same function. */
int HitCheckBox(void *position, void *map_unit)
{
    HitProbe *probe = (HitProbe *)position;
    HitMapUnit *unit = (HitMapUnit *)map_unit;
    HitAlignedBounds bounds = unit->bounds;
    HitVector *axis = unit->direction;
    HitAlignedVector unit_position = unit->position;
    float z_delta = probe->position.z - unit_position.value.z;
    float expansion = probe->size * 1.5f;
    float x_delta = probe->position.x - unit_position.value.x;
    float projection = x_delta * axis->x + z_delta * axis->z;
    float nearest_x;
    float nearest_z;
    float radius;
    float distance_x;
    float distance_z;
    float distance;

    if (bounds.value.radius + expansion < __builtin_fabsf(projection)) {
        return 0;
    }
    nearest_x = projection * axis->x + unit_position.value.x;
    nearest_z = projection * axis->z + unit_position.value.z;
    radius = bounds.value.corner_radius + expansion;
    distance_x = probe->position.x - nearest_x;
    distance_z = probe->position.z - nearest_z;
    distance = distance_x * distance_x + distance_z * distance_z;
    if (distance < radius * radius) {
        return 1;
    }
    return 0;
}

/* No prototype on purpose: the original calls this with ONE argument
   though it takes two, leaving $a1 to carry the map unit by luck of the
   register allocator. A full prototype makes gcc emit `move a1,s0`. */
extern int HitCheckBoxUwamono();
extern float D_004D7F48;
extern float D_004D7F44;

/* Does this probe stand inside the "uwamono" (overlay object) at unit?
   Height band first, then the shape's own container kind picks a circle or
   a box test. Case 2 tail-calls with the untouched $a0, so the probe needs
   a second copy; gcc puts that copy in $a1 because `unit_` has already
   been moved to $s0 by then, while the original still had $a1 busy and
   used $a2. Neither declaration order nor statement order moves it --
   swept probe-first, unit-first, probe-declared-last and hoisting the
   first field read above the second local. */
int HitCheckUwamonoAt(void *position, void *unit_)
{
    /* $a2: the second copy of the probe pointer -- see above. */
    PIN(HitProbe *probe, "$6");
    HitMapUnit *unit;
    HitMapUnitShape *shape;
    float probe_y;
    float unit_y;

    probe = (HitProbe *)position;
    unit = (HitMapUnit *)unit_;
    shape = &unit->shape;
    probe_y = probe->position.y;
    unit_y = unit->position.value.y;

    if (probe_y < unit_y) {
        goto miss;
    }
    if (unit_y + unit->bounds.value.height < probe_y) {
        goto miss;
    }
    switch (shape->active) {
    case 1:
        if (CheckDist2D(&probe->position, &unit->position) <
            unit->bounds.value.radius + D_004D7F44) {
            return 1;
        }
        return 0;
    case 2:
        return HitCheckBoxUwamono(probe);
    }
miss:
    return 0;
}

/* The oriented-box test used by the uwamono path: project the probe onto
   the unit's axis, reject outside the half-length, then compare the
   perpendicular distance against the corner radius. */
/* The oriented-box test used by the uwamono path: project the probe onto
   the unit's axis, reject outside the half-length, then compare the
   perpendicular distance against the corner radius.

   PARKED at 30 diffs, right length (49 words) and right control flow.
   $a2 must be pinned or the map unit stays in $a1, the `move a2,a1` in the
   original's first slot disappears and the whole thing runs one word
   short. The bounds copy must be DECLARED first (it owns sp+0) but READ
   through `unit->bounds` (the original never reloads the stack copy, only
   writes it). What is left is the same open question as HitCheckBox in
   this file: the 16-byte union copies come out low-word-first for the
   position and the FP temporaries land one register off throughout
   ($f0/$f1 and $f8/$f10 swapped against the original).

   Swept here: bounds-first vs position-first declaration and copy order,
   stack-copy vs unit reads for both radii, and all eight orderings of the
   z/x pairs (deltas, nearest, distances) -- 30 is the floor, and the
   nearest_x-before-nearest_z order below is what buys the last two. */
int HitCheckBoxUwamono(void *position, void *map_unit)
{
    /* $a2 -- see above. */
    PIN(HitMapUnit *unit, "$6");
    HitProbe *probe = (HitProbe *)position;
    HitAlignedBounds bounds;
    HitAlignedVector unit_position;
    HitVector *axis;
    float z_delta;
    float x_delta;
    float projection;
    float nearest_x;
    float nearest_z;
    float radius;
    float distance_x;
    float distance_z;
    float distance;

    unit = (HitMapUnit *)map_unit;
    unit_position = unit->position;
    axis = unit->direction;
    bounds = unit->bounds;
    z_delta = probe->position.z - unit_position.value.z;
    x_delta = probe->position.x - unit_position.value.x;
    projection = x_delta * axis->x + z_delta * axis->z;

    if (unit->bounds.value.radius + D_004D7F48 < __builtin_fabsf(projection)) {
        return 0;
    }
    nearest_x = projection * axis->x + unit_position.value.x;
    nearest_z = projection * axis->z + unit_position.value.z;
    radius = unit->bounds.value.corner_radius + D_004D7F48;
    distance_z = probe->position.z - nearest_z;
    distance_x = probe->position.x - nearest_x;
    distance = distance_x * distance_x + distance_z * distance_z;
    if (distance < radius * radius) {
        return 1;
    }
    return 0;
}

extern int NyuruCircle();
extern int NyuruMatrix();

typedef struct HitNyuruProbe {
    char pad_000[0x10];
    HitAlignedVector position;      /* 0x10 */
    char pad_020[0x10];
    HitAlignedVector move;          /* 0x30 */
    char pad_040[0x9C0];
} HitNyuruProbe;

/* Slide the probe along the unit's "nyuru" shape, restoring the probe and
   cancelling the move when nothing was hit. */
int HitCheckNyuru(HitNyuruProbe *probe, HitMapUnit *unit)
{
    HitAlignedVector savePosition;
    HitAlignedVector saveMove;
    int nUnit;
    /* $v0: the call result is tested and then reused as the return value
       0; anywhere else costs a `move` after each call and another to set
       up the return. */
    PIN(int nOk, "$2");

    savePosition = probe->position;
    saveMove = probe->move;
    nUnit = -1;
    switch (unit->shape.active) {
    case 1:
        nOk = NyuruCircle(probe, unit);
        break;
    case 2:
        nOk = NyuruMatrix(probe, unit, 0);
        break;
    default:
        goto done;
    }
    if (nOk == 0) {
        return 0;
    }
    nUnit = HitCheckMapUnit(probe);
done:
    nOk = -1;
    if (nUnit != nOk) {
        probe->position = savePosition;
        probe->move = saveMove;
        probe->position.value.x = probe->position.value.x - probe->move.value.x;
        probe->position.value.z = probe->position.value.z - probe->move.value.z;
        /* x before z: gcc reverses this pair of independent zero stores. */
        probe->move.value.x = 0.0f;
        probe->move.value.z = 0.0f;
        return 0;
    }
    return 1;
}

/* Called with the function's own two parameters, untouched in $a0/$a1 --
   which is why gcc copies them into $t0/$a2 for local use. */
extern int HitCheckCorner(void *position, void *map_unit);

/* Test a circular probe against one oriented map-unit box, rounding the
   two end caps: inside the flat part is a plain hit, but a probe that
   projects past the flat part and lands outside the cap radius is handed
   to HitCheckCorner for the exact corner test.

   PARKED at 47 of 69 words. The shape is right -- same length, same
   branches, the same `sltu v0,zero,v0` tail and the same argument-less
   call site -- and what is left is the FP register numbering (ours runs
   one lower throughout: $f10/$f9 for the probe's x/z where retail uses
   $f11/$f10) plus the word order inside the two 16-byte copies. That is
   the SAME wall as HitCheckBox above: retail emits the quadword copy
   high-word-first (`ld 24` before `ld 16`) and no source spelling found
   so far produces that for the position copy.

   Solved and not to be re-swept: `bounds` must be DECLARED first (it owns
   sp+0) but ASSIGNED after `unit_position` (retail copies the position
   into sp+16/24 first) -- declaring it first and initialising it there
   puts the position at sp+0, and declaring it second puts the bounds
   there. Splitting the initialiser is what gets both. The call must pass
   `position, map_unit` explicitly: that is what keeps $a0/$a1 live and
   makes gcc copy them to $t0/$a2, worth 2 words. `past_end` gets $v1
   where retail has $t1, because retail materialises the return-0 in $v0
   at the top of the function; a `result` local returned from the early
   exits instead only adds a `move v0,result` at the tail (70-74 words in
   three tried shapes). */
int HitCheckBoxCorner(void *position, void *map_unit)
{
    HitProbe *probe = (HitProbe *)position;
    HitMapUnit *unit = (HitMapUnit *)map_unit;
    HitAlignedBounds bounds;
    HitAlignedVector unit_position = unit->position;
    HitVector *axis = unit->direction;
    float z_delta = probe->position.z - unit_position.value.z;
    float expansion = probe->size * 1.5f;
    float x_delta = probe->position.x - unit_position.value.x;
    float projection;
    int past_end;
    float nearest_x;
    float nearest_z;
    float radius;
    float distance_x;
    float distance_z;
    float distance;

    bounds = unit->bounds;
    projection = x_delta * axis->x + z_delta * axis->z;
    if (bounds.value.radius + expansion < __builtin_fabsf(projection)) {
        return 0;
    }
    past_end = 0;
    if (bounds.value.radius < __builtin_fabsf(projection)) {
        past_end = 1;
    }
    nearest_x = projection * axis->x + unit_position.value.x;
    nearest_z = projection * axis->z + unit_position.value.z;
    radius = bounds.value.corner_radius + expansion;
    distance_x = probe->position.x - nearest_x;
    distance_z = probe->position.z - nearest_z;
    distance = distance_x * distance_x + distance_z * distance_z;
    if (distance < radius * radius) {
        if (bounds.value.corner_radius * bounds.value.corner_radius <
            distance) {
            if (past_end == 1) {
                return HitCheckCorner(position, map_unit) != 0;
            }
        }
        return 1;
    }
    return 0;
}

/* Statement order here is load-bearing, and was worth 79 of the 84 words:
   each corner's X must be assigned BEFORE its Y (writing Y first, which
   is the order the stores come out in, costs 15 words per corner), and
   `axis` must be read BETWEEN the position and bounds copies -- reading it
   after both leaves gcc's `lw` for it after the four `sd`s instead of
   before them. Only the bounds/corner values have to be re-read from the
   copy at every use: hoisting them into `radius`/`corner` locals makes gcc
   keep them in callee-saved $f20/$f21 across the four calls and grows the
   frame by 16 bytes.

   The last word is that same `lw axis` still one slot late; no source
   order tried put it before the stores, so it is rotated into place by
   --rotate HitCheckCorner:17:5 (a pure 5-instruction window rotation of
   four independent stores and one independent load).

   The exact rounded-corner test HitCheckBoxCorner falls back to: build the
   four corner points of the unit's box (the radius axis and the corner
   axis, and the same pair rotated a quarter turn) and accept the probe if
   it is within its own expanded radius of any of them. */
int HitCheckCorner(void *position, void *map_unit)
{
    HitProbe *probe = (HitProbe *)position;
    HitMapUnit *unit = (HitMapUnit *)map_unit;
    HitVector c0;
    HitVector c1;
    HitVector c2;
    HitVector c3;
    HitAlignedVector unit_position;
    HitAlignedBounds bounds;
    HitVector *axis;

    unit_position = unit->position;
    axis = unit->direction;
    bounds = unit->bounds;

    c0.x = unit_position.value.x + (bounds.value.radius * axis[0].x + bounds.value.corner_radius * axis[2].x);
    c0.y = unit_position.value.y;
    c0.z = unit_position.value.z + (bounds.value.radius * axis[0].z + bounds.value.corner_radius * axis[2].z);
    if (CheckDist2D(&probe->position, &c0) < probe->size * 1.5f) {
        return 1;
    }
    c1.x = unit_position.value.x - (bounds.value.corner_radius * axis[0].z + bounds.value.radius * axis[2].z);
    c1.y = unit_position.value.y;
    c1.z = unit_position.value.z + (bounds.value.corner_radius * axis[0].x + bounds.value.radius * axis[2].x);
    if (CheckDist2D(&probe->position, &c1) < probe->size * 1.5f) {
        return 1;
    }
    c2.x = unit_position.value.x - (bounds.value.radius * axis[0].x + bounds.value.corner_radius * axis[2].x);
    c2.y = unit_position.value.y;
    c2.z = unit_position.value.z - (bounds.value.radius * axis[0].z + bounds.value.corner_radius * axis[2].z);
    if (CheckDist2D(&probe->position, &c2) < probe->size * 1.5f) {
        return 1;
    }
    c3.x = unit_position.value.x + (bounds.value.corner_radius * axis[0].z + bounds.value.radius * axis[2].z);
    c3.y = unit_position.value.y;
    c3.z = unit_position.value.z - (bounds.value.corner_radius * axis[0].x + bounds.value.radius * axis[2].x);
    if (CheckDist2D(&probe->position, &c3) < probe->size * 1.5f) {
        return 1;
    }
    return 0;
}
