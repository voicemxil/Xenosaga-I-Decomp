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

extern int HitCheckBoxUwamono(void *position);
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
