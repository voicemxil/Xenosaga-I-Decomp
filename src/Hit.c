/* Collision-query helpers used by map-unit and actor game logic. */

extern int HitCheckMapUnitPosSize(void *unit, void *position, float size);

/* Test a point against a map unit with no radius expansion */
int HitCheckMapUnitPos(void *unit, void *position)
{
    return HitCheckMapUnitPosSize(unit, position, 0.0f);
}
