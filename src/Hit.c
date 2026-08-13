/* Collision-query helpers used by map-unit and actor game logic. */

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
    char pad_0C0[0xE4];
    char container;
    char pad_1A5[0x28];
    char active;
    char pad_1CE[0x132];
} HitMapUnit __attribute__((aligned(8)));

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
        if (unit->active == 0) {
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
        if (unit->active == 0) {
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
        if (unit->active == 0) {
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
        if (unit->active == 0) {
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

/* Find a collidable map unit outside one excluded container slot. */
/* TODO: LENGTH near-match: gcc folds the interior D_0047AEC0 cursor and the
 * record-base pointer together, producing 65 words versus the original 66 and
 * changing the saved-register assignment for the excluded slot and masks. */
int HitCheckContainerPosSize(void *position, int excluded, float size)
{
    HitProbe probe;
    char *tail = D_0047AEC0;
    int i = 0;
    int offset = 0;
    HitMapUnit *base = (HitMapUnit *)(D_0047AEC0 - 0x1A0);

    probe.position = *(HitVector *)position;
    probe.size = size;
    while (i < 0x40) {
        HitMapUnit *unit = (HitMapUnit *)((char *)base + offset);

        if (*(short *)(tail - 0xFC) != -1 &&
            *(char *)(tail + 4) == 0 &&
            i != excluded &&
            *(char *)(tail + 0x2D) != 0 &&
            (*(unsigned int *)(tail - 0x1A0) & 0x10000) != 0 &&
            HitCheckMapUnitPosAt(&probe, unit) != 0) {
            return i;
        }
        i++;
        tail += 0x300;
        offset += 0x300;
    }
    return -1;
}

/* Find a collidable map unit belonging to another team. */
/* TODO: REGISTER near-match: the natural byte-base-plus-offset expression emits
 * addu $a1,$fp,$s3; the original's sole difference is addu $a1,$s3,$fp. */
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
        unit = (HitMapUnit *)((char *)base + offset);
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
    int result;
    float distance;

    if (unit->position.value.y + unit->bounds.value.height * 0.9f <
        probe->position.y) {
        result = 0;
    } else if (probe->position.y < unit->position.value.y - 1.8f) {
        result = 0;
    } else {
        switch (unit->active) {
        case 1:
            distance = CheckDist2D(&probe->position, &unit->position.value);
            result = distance < probe->size + unit->bounds.value.radius;
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
    int result;
    float distance;

    if (unit->position.value.y + unit->bounds.value.height * 0.9f <
        probe->position.y) {
        result = 0;
    } else if (probe->position.y < unit->position.value.y - 1.8f) {
        result = 0;
    } else {
        switch (unit->active) {
        case 1:
            distance = CheckDist2D(&probe->position, &unit->position.value);
            result = distance < probe->size + unit->bounds.value.radius;
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
/* TODO: LOGIC near-match: the aligned aggregate copies now give the original
 * 51-word length, but gcc schedules the two local copies and floating-point
 * operand loads differently, cascading through 29 encoded instructions. */
int HitCheckBox(void *position, void *map_unit)
{
    HitProbe *probe = (HitProbe *)position;
    HitMapUnit *unit = (HitMapUnit *)map_unit;
    HitAlignedBounds bounds = unit->bounds;
    HitAlignedVector unit_position = unit->position;
    HitVector *axis = unit->direction;
    float z_delta = probe->position.z - unit_position.value.z;
    float x_delta = probe->position.x - unit_position.value.x;
    float projection = x_delta * axis->x + z_delta * axis->z;
    float expansion = probe->size * 1.5f;
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
