# MenuItemSegmentMain recovery map

`MenuItemSegmentMain` is the segment/robot-part page driver in the item menu.
It is 0x1280 bytes (1184 instructions), from `0x0028C750` through
`0x0028D9CC`. The function is not one flat UI operation: it is a six-state
lifecycle wrapped around one shared renderer. This note records the
assembly-supported structure so the C reconstruction can be built and checked
one state at a time.

No behavioral implementation should be registered until the whole function is
represented. All states converge on shared rendering, so a partial driver can
look plausible while silently omitting animation and print-buffer updates.

## Top-level work layout

`MenuItemSegment` is a pointer to the page work block. The typed prefix and
major embedded regions recovered from constant offsets are:

```c
typedef struct {
    unsigned char state;       /* 0x0000 */
    unsigned char draw;        /* 0x0001 */
    short slideX;              /* 0x0002 */
    short baseY;               /* 0x0004 */
    short pad0006;
    void *texture;             /* 0x0008 */
    int selected;              /* 0x000C, wrapped to 0..17 */
    char windows[0x328];       /* 0x0010, two 0x194 WindowDX objects */
    char headingSprites[0x50]; /* 0x0338, two 0x28 sprites */
    char headingMessages[0x88];/* 0x0388, two 0x44 messages */
    SEGMENT_ENTRY entry[18];   /* 0x0410, stride 0x15C */
    char pad1C88[4];
    unsigned short allPartMask;      /* 0x1C8C */
    unsigned short selectedPartMask; /* 0x1C8E */
    char partSprites[0x2F8];   /* 0x1C90 */
    char ribbonArea[0x188];    /* 0x1F88 */
    char printArea[0x130];     /* 0x2110 */
    void *printTexture;        /* 0x2240 */
} SEGMENT_WORK;
```

The last proven access is the word at 0x2240, so the work block is at least
0x2244 bytes. The initializer in `MenuItem` allocates this block from the
menu work arena; it should eventually confirm the exact tail size.

## Entry layout and flag banks

There are exactly 18 entries. GCC repeatedly forms `index * 0x15C`, and the
initializer advances its entry pointer by 0x15C for 18 iterations.

```c
typedef struct {
    unsigned char pulse;       /* 0x00, highlight intensity */
    signed char pulseStep;     /* 0x01 */
    char pad02[2];
    unsigned char flags;       /* 0x04, bits 0..2 */
    char pad05;
    unsigned char partPulse;   /* 0x06 */
    signed char partPulseStep; /* 0x07 */
    char mainSprite[0x28];     /* 0x08 */
    char flag1Sprite[0x28];    /* 0x30 */
    char flag2Sprite[0x28];    /* 0x58 */
    char allFlagsSprite[0x28]; /* 0x80 */
    char padA8[0x28];
    char number[0x8C];         /* 0xD0 */
} SEGMENT_ENTRY;
```

For entry index `i` (0..17), initialization clears `flags` then performs:

- `xglFlagsGet(0xC81 + i, 1)` -> bit 0;
- `xglFlagsGet(0xC95 + i, 1)` -> bit 1;
- `xglFlagsGet(0xCA9 + i, 1)` -> bit 2.

For entries with bit 2, `subRoboPartsCheck(i + 1)` is ORed into
`allPartMask`. `selectedPartMask` is refreshed with
`subRoboPartsCheck(selected + 1)` while the page is active.

The two byte animations are bounded triangular waves. `pulse` and
`partPulse` move between 0 and 0x78 by their signed step; the step is negated
at an endpoint. Selection changes force the selected entry's `partPulseStep`
to +4 if it is currently zero, while non-selected entries are driven back to
zero.

## State dispatch

The jump table `jtbl_004C50F0` maps `work->state` exactly:

| State | Block | Meaning established by effects |
|---:|---|---|
| 0 | `0x0028C7A8` | initialize every embedded widget and set state 2 |
| 1 | `0x0028CCC4` | no state-local work; run shared rendering if enabled |
| 2 | `0x0028CB24` | wait for global menu byte `D_0036C183 == 0x30`, then enter |
| 3 | `0x0028CB50` | slide `slideX` toward 0x10 with `MoveSlide(..., 3.0f)` |
| 4 | `0x0028CBA8` | active directional selection and selected-part refresh |
| 5 | `0x0028CC88` | slide `slideX` toward -0x200, then return to state 2 |

Out-of-range states skip directly to the shared-render gate. State 0 ends by
writing state 2. State 3 writes state 4 when the slide reaches 0x10, or state
5 when the global menu byte is no longer 0x30. State 5 clears `draw` and
returns to state 2 after reaching -0x200.

In state 4, the halfword `D_00490DC4` decodes to a signed selection delta:

| Input mask | Delta |
|---:|---:|
| `0x1000` | -3 |
| `0x2000` | +1 |
| `0x4000` | +3 |
| `0x8000` | -1 |

The result wraps modulo 18 and plays sound effect 3 when it changes. If
`MenuWork[3] != 0x30`, state 5 is requested. The page also copies
`selected + 1` to `MenuWork[0x74]`.

## Initialization structure

State 0 writes the stable page prefix:

- `slideX = -0x200`, `baseY = 0x34`;
- `texture = 0x000FFFF8`;
- `draw = 0`, `selected = 0`;
- `entry[i].flags` from the three flag banks above;
- `allPartMask` and `selectedPartMask` reset before reconstruction;
- pulse seed `entry[0].pulse = 0x20`, step +4;
- two heading windows, sprites and messages;
- state becomes 2.

The 19 sprite ids beginning at work offset 0x1C90 come from the 38-byte
halfword table `D_004C5018`:

```c
{ 0x1315, 0x130E, 0x1308, 0x130A, 0x130A, 0x130C, 0x130C,
  0x1311, 0x1314, 0x1313, 0x1310, 0x1312, 0x130F, 0x130D,
  0x1307, 0x1309, 0x1309, 0x130B, 0x130B }
```

The four sprites in every 0x15C entry use ids 0x1306, 0x1305, 0x1304 and
0x1303; the main icon chooses 0x1301 only when `flags == 7`, otherwise 0x1300.

## Shared renderer

Rendering is gated by `work->draw`. If it is zero the function returns after
the state transition. Otherwise the common tail performs, in order:

1. Update and draw the two embedded `WindowDX` objects using `slideX`,
   `baseY`, and `texture`.
2. If the selected entry has flag bit 0, update the map/location message via
   `MenuSegmentMapNameGet` and `eMessageMain`.
3. If flag bit 2 is set, choose the item/name text using the second dispatch
   table, copy the description, and run the info message. Otherwise clear the
   first byte of the info buffer from `D_004DAAD0`.
4. Cache `WindowTexAddrGet(2)` at 0x2240 and call `endPrintExtFunc` for the
   page print context.
5. Animate/draw all 18 entry rows. The row is `index / 3`; the column is
   `index % 3`, which explains the explicit `div` and the grid coordinate
   arithmetic.
6. Build and submit two print groups for the entry grid.
7. Update the ribbon and calculate the common anchor at 0x1C94/0x1C96.
8. Draw up to six robot-part indicator groups according to `allPartMask` and
   `selectedPartMask`.
9. Draw the two headings, submit print context 0x0F, then run `eNumberMain`
   for all 18 entries.

The renderer contains explicit 128-bit saves/restores (`sq`/`lq`) around
`endPrintExtFunc`. Those are caller-state preservation selected by GCC for a
high-register-pressure natural-C loop, not evidence that the function itself
is handwritten.

## Secondary item-name dispatch

When the selected entry has flag bit 2, `selected` is dispatched through
`jtbl_004C5110` for values 0..8:

| Selected | Special mask | Item id | Event fallback id |
|---:|---:|---:|---:|
| 0 | `allPartMask & 0x40` | 0x13 | 0x38 |
| 1 | `allPartMask & 0x80` | 0x15 | 0x3B |
| 2 | `allPartMask & 0x200` | 0x16 | 0x3C |
| 3,4,5 | none | `selected + 1` | none |
| 6 | `allPartMask & 0x100` | 0x14 | 0x39 |
| 7 | `allPartMask & 0x200` | 0x16 | 0x3D |
| 8 | `allPartMask & 0x100` | 0x14 | 0x3A |

The special path calls `MenuSegmentItemNameGet(itemId)` and
`MenuSegmentInfoTextGet(itemId)`. When its mask bit is absent, it calls
`dataEvtItmNameGet(fallbackId)` for the display name and gets description text
from `MenuSegmentInfoTextGet(selected + 1)`. Values above 8 use the ordinary
`MenuSegmentItemNameGet(selected + 1)` path.

## Static initializer tables

The renderer copies these rodata blocks to local arrays. Their exact byte
shapes are important because GCC emits recognisable inline copies:

- `D_004C5060`: 12 bytes of six signed halfwords;
- `D_004C5070`: 12 bytes of six signed halfwords;
- `D_004C5080`: 12 bytes of six signed halfwords;
- `D_004C5090`: 12 bytes of six signed halfwords;
- `D_004C50A0`: 0x48 bytes of 36 signed halfwords;
- `D_004C5018`: 0x26 bytes of 19 sprite-id halfwords.

`tag_msg.5` is a two-pointer table containing `"Location"` and
`"Rare item"`. `D_004DAAF0` contains four byte-sized heading sprite ids
`{ 0x16, 0x13, 0x17, 0x13 }`, and `D_004DAAF8` begins with message id
0xBEB2.

## Recommended implementation order

1. Compile-time-check the recovered work and entry offsets.
2. Reconstruct state 2 and states 3/5; they are compact and use already
   understood `MoveSlide` idioms.
3. Reconstruct state 4 and confirm its modulo-18 selection shape.
4. Reconstruct state 0 from the typed entry array and literal tables.
5. Add the shared renderer in the nine phases listed above.
6. Compare the whole function before tuning declarations and store order.
   The jump-table `addu` versus `daddu` post-assembly issue already documented
   for `MenuBoxChk` may recur here and should not be mistaken for bad logic.

## Current verification blocker

During initial recovery the old Colima instance entered an inconsistent state:
the host had 3.2 GiB free and 34 million free inodes, but Docker first reported
a stopped container as running and then failed reading the container `hosts`
file with an I/O error. This was not a disk-capacity diagnosis. The VM/toolchain
has since been repaired; the layouts and future implementation should be
checked through the normal container before registration.
