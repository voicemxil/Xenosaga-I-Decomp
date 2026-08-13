# JNT_getVal2 recovery map

`JNT_getVal2` is the central animated-joint record evaluator. It is large
(0x151C bytes / 1351 instructions), but its control flow separates cleanly
into a common decoder, seven record modes, and one shared epilogue. This note
records the verified assembly structure so the C reconstruction can proceed
one mode at a time.

## Contract

The call sites establish this effective signature:

```c
JNT_ANIM_RECORD *JNT_getVal2(JNT_WORK *work, JNT_ANIM_RECORD *record,
                             float frame);
```

- `work` is the existing scratchpad-backed `JNT_WORK`.
- Every input record is exactly 0x40 bytes.
- The return value is always `record + 1` (the original returns `a1 + 0x40`).
- `work->nIndex` at offset 0x2C is incremented in the shared epilogue.
- `record->type` is the unsigned halfword at offset 0x00.
- `record->matrixIndex` is the unsigned halfword at offset 0x0E.
- Record payload vectors begin at offsets 0x10 and 0x20.

Callers retain the returned pointer and repeatedly invoke the evaluator until
the model's element count is reached. This is the animated counterpart of
`JNT_getStaticVal2`, which uses the same record-walking convention.

## Attribute decode

Unless `work->nFlags & 4` selects `dummyAttr`, the function begins with:

```c
attr = FCV2_getPackAttribute((char *)work + 4);
```

The observed attribute fields are:

```c
typedef struct {
    unsigned short unknown00;
    unsigned short unknown02;
    unsigned short flags;       /* 0x04 */
    unsigned short channels;    /* 0x06, consumed three bits at a time */
    unsigned short matrixT;     /* 0x08 */
    unsigned short unknown0A;
    unsigned short matrixR;     /* 0x0C */
} JNT_FCV_ATTRIBUTE;
```

`flags == 0xFFFF` terminates curve evaluation: bit 4 is set in
`work->nFlags`, and the function returns the next record without dispatching.

Attribute flag `0x8000` enables the shared root-TRS predecode. It consumes
translation, rotation, and scale channel groups with `get_fcvpack`,
`get_fcvpack_fix`, and `get_fcvpack_flag`, then copies the decoded values from
work offsets 0x70/0x80/0x90 into the root TRS fields at 0x40/0x50/0x60.

The channel mask is shifted right by three after each vector group. Individual
bits select which components call `FCV2_getPackValue`; absent scale components
retain 1.0 through `get_fcvpack_fix`.

## Record dispatch

The jump table at `jtbl_004D1A60` maps the seven record types exactly:

| Type | Assembly block | Current interpretation |
|---:|---|---|
| 0 | `0x00312148` | no record-specific transform; shared epilogue only |
| 1 | `0x00310EB8` | standard animated TRS plus optional T/R matrix references |
| 2 | `0x00311160` | bounded/chained transform with per-axis limits |
| 3 | `0x00311098` | direct TR transform and matrix snapshot |
| 4 | `0x003112E8` | articulated alignment/constraint solver (largest mode) |
| 5 | `0x003120A8` | generic TRS transform |
| 6 | `0x00312148` | no record-specific transform; shared epilogue only |

Types outside 0..6 skip directly to the shared post-transform path.

### Type 1: standard animated TRS

This mode establishes the baseline source shape:

1. Load the matrix selected by `record->matrixIndex` from `work->pMatrix`.
2. Initialize decoded translation/rotation W components to 1.0.
3. Decode translation and rotation through successive three-bit channel groups.
4. Decode scale components individually with `FCV2_getPackValue` when their
   channel bits are set.
5. Apply translation, rotation, and scale through the `CUR_MATRIX_*` helpers.
6. Optional attribute bits 0x4 and 0x8 replace translation or rotation from
   referenced matrices (`matrixT` and `matrixR`).
7. If the record has an auxiliary relative pointer at offset 0x04, append its
   referenced data to the producer table rooted at `work->pProducer`.
8. Store the resulting matrix in the current matrix-palette slot.

### Type 3: direct TR snapshot

This compact mode is a useful first implementation target. It loads the
record's matrix, decodes translation and rotation, applies T then R, saves a
copy at work offset 0x4D0, writes the current palette matrix, clears the work
field at 0x804, initializes the embedded 0xB0 transform from the saved matrix,
and sets the embedded transform count at 0x4C0 to one.

### Type 4: articulated constraint solver

The apparent monolith from 0x003112E8 through 0x003120A8 is one record mode,
not several functions. Its repeated structure is:

- walk matrices in 0x40-byte strides;
- derive normalized direction vectors using `sqrtf`;
- compute yaw/roll terms with `xglAtan2`;
- build alignment matrices with `alignXAxis` and `MATRIX_transpose3`;
- apply the aligned transform back through `CUR_MATRIX_Set/Get/4s3`.

There are ten `sqrtf`, four `xglAtan2`, five `alignXAxis`, and five
`MATRIX_transpose3` calls. These repetitions should become small natural-C
vector helpers before attempting compiler scheduling, rather than being copied
as one flat block.

## Shared epilogue

If the initial attribute had flag 0x8000, the shared tail additionally:

1. Resets the matrix at work offset 0x7B0 to identity.
2. When `(work->nCurveId & 7) == 0`, computes the XYZ difference between the
   record's referenced matrix and the current output matrix.
3. Stores the negated difference at work offsets 0x7E0..0x7E8.
4. Copies the referenced matrix translation into the current output matrix.

Finally it increments `work->nIndex` and returns `record + 0x40`.

## Helper contracts inferred from EABI argument placement

```c
void get_fcvpack(float *out, void *channel, JNT_WORK *work,
                 unsigned int mask, float frame);
void get_fcvpack_fix(float *out, JNT_WORK *work, unsigned int mask,
                     float defaultValue, float frame);
void get_fcvpack_flag(JNT_WORK *work, unsigned int mask, float frame);
```

The last floating arguments occupy `$f12/$f13`, explaining why these helpers
look under-parameterized if only `$a0..$a3` are considered.

## Implementation order

1. Add typed `JNT_ANIM_RECORD` and `JNT_FCV_ATTRIBUTE` layouts.
2. Implement the common attribute decode and return contract.
3. Implement modes 3 and 5 first; both are compact compositions of already
   matched `CUR_MATRIX_*` helpers.
4. Implement mode 1 and validate its channel-mask consumption.
5. Factor vector normalization/alignment helpers from mode 4's repeated blocks.
6. Finish mode 2 limits and mode 4 articulated constraints.
7. Only after behavior and instruction count agree, tune declaration and store
   order for exact GCC 2.96 scheduling.
