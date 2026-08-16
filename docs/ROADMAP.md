# Roadmap

State as of 2026-08-14: ~1,663+ functions verified (~12.0%), the libc/libgcc
infrastructure sprint essentially complete. This file maps what remains and
in what order it is likely to pay off. Byte counts are unmatched bytes from
`Repo.orig_functions` grouped by symbol prefix.

## How the libc sprint worked (recipe recap)

Everything below builds on the same machinery, all committed:

- **Vendored real sources** compile byte-exact: newlib at CVS rev
  `5bacbf109` (2000-08-16; signal.c at `b944c6f66`, mprec.c at the 1.9.0
  rev) under `newlib/`, gcc frame.c/libgcc2.c/fp-bit.c at rev
  `ec6bfc9b7ca` (1999-12-29) under `newlib/gccsrc/`. History mirrors:
  github.com/mirror/newlib-cygwin, github.com/gcc-mirror/gcc.
- **Flags**: game code 2.96 `-O2 -G8`; libc `-O2 -G0`; libgcc adds
  `-mlong32`; SDK (sce*) 2.9 `-O2 -G0 -fno-schedule-insns`.
- **Fixer passes** (`tools/fix_cc_asm.py`, all opt-in per file):
  `--barrier-return-store`, `--barrier-branch-move` (gas delay-slot
  theft), `--expand-sym-loads` (ctype-style symbol loads),
  `--unfill-gcc-slots` (exists, needs per-site selection),
  `--no-fill-delay` (rarely right). Reordering family:
  `--swap-adjacent`, `--rotate FUNC:N[:LEN]` (negative LEN rotates
  left), `--rotate-seq` (same sites applied one at a time with indices
  re-resolved, for OVERLAPPING windows -- order matters),
  `--swap-into-slot`, `--swap-slot-target`; all of these are checked by
  `tools/audit_swaps.py`, which recompiles without them and fails the
  build if the instruction multiset changed (i.e. if a "reorder" is
  really masking different logic). Register naming: `--swap-regs
  FUNC:A-B`, GPR or FP (`f0-f1`). Idiom rewrites: `--zero-quad-store`
  (gcc's `por $X,$0,$0` + `sq $X` for a TImode zero; the original built
  `nop` + `sq $0`).
- **Sony glue is hand-written from disasm** when the source never existed
  publicly: `malloc_lock.c`, `gccsrc/gthr-ps2.h` (WaitSema/SignalSema on
  `__sce_sema_id`/`__sce_eh_sema_id`).

## Near-term (days)

1. **Port from SquareMan/xenosaga** (~/Claude/xenosaga-squareman): 37
   function definitions there are not in our decompiled.txt — notably
   `src/game/eMessage.c` (14 fns, a subsystem we have not touched),
   `src/xgl/math.c` (6), `src/game/JS.c` (5), `src/xgl/thread.c` (4).
   Same game, so their C should match under our pipeline after flag
   triage. Also steal structural ideas from their `include/` tree.
2. **Hard tails with documented state** (see decompiled.txt TODOs):
   `_vfprintf_r` (369d, needs per-site slot unfill), `_strtod_r` (674d,
   duplicated tail block), `_malloc_r` (350d), `_free_r` (11d),
   `__unpack_f`/`__pack_f`, ctors/dtors (Sony `DO_GLOBAL_DTORS_BODY`
   with atexit-chain drain), frame.c tails (`find_fde` 20d,
   `__frame_state_for` 51d, `end_fde_sort` 131d, `execute_cfa_insn`
   214d — all regalloc/scheduling shapes).
3. **Permuter revival**: the regalloc-class tails are exactly what
   decomp-permuter automates. A copy lives in the recovered image
   (excluded from the running container to save space) and in
   squareman's tools/. Setting up per-function harnesses would convert
   most remaining "hard tails" into compute time.

## Mid-term: subsystem ranking (unmatched bytes)

| Subsystem | Unmatched | Notes |
|-----------|-----------|-------|
| Menu*     | 170KB (99%) | The monster. Menu screens; repetitive shapes once the first few land. See docs/MENU_ITEM_SEGMENT_RECOVERY.md |
| xgl*      | 60KB (82%) | Engine layer; 18 files exist in src/xgl with established types |
| nml*      | 55KB (91%) | Model/filter/packet; 3 files exist |
| tsk*      | 40KB (98%) | Task/screen state machines (src/game/tsk.c started); many in the ov02 overlay — use tools/disasm.py, asm/ov02 is a placeholder |
| sef*      | 33KB (95%) | Sound effects driver |
| Game*     | 30KB (94%) | Core game loop family |
| JNT_*     | 26KB (96%) | Joint/animation; see docs/JNT_GETVAL2_RECOVERY.md |
| Umn*/Char*/Check*/ACT* | ~72KB | Field/actor logic; files established, steady grinding |
| Paus*/Agws*/Draw*/Init*/sub* | ~66KB | Untouched families, greenfield |
| sce*      | 18KB (99%) | SDK: needs the 2.9 compiler recipe per TU; sceVif1Pk near-misses in progress |
| Java_*    | 13KB (31% left) | JVM natives, mostly done — finish the tail |

## Structural work (squareman-style scaffolding)

The src/ tree now mirrors squareman's layout (game/sdk/xgl/nml/ssd/libc/
libgcc). The next structural steps, in order of value:

1. **Shared type headers**: today every game TU re-models structs it
   touches (ACTOR in ACT.c, XGL_TASK in tsk.c, ...). Introduce
   `include/game/` headers (actor.h, task.h, msg.h...) that TUs adopt
   incrementally — new decomp work stops re-deriving layouts. Byte
   matching is unaffected by header organization; offsets stay pinned
   by the struct definitions.
2. **A common.h** for the universal typedefs (u8/s16/etc.) instead of
   per-file `typedef unsigned int size_t;` repetition.
3. **objdiff.json** is already generated; keeping subsystem grouping in
   configure.py aligned with src/ dirs makes objdiff navigation match
   the tree.

## Provenance ledger (what we know for certain)

- Game code: ee-gcc 2.96-ee-001003-1, -O2 -G8 (libm at -G0).
- SDK (sce*, mpeg): ee-gcc 2.9-ee-991111, -O2 -G0 -fno-schedule-insns.
- libc: newlib snapshot ≈ late September 2000, built at -O2 -G0 with
  2.96; PS2 syscalls return int; malloc aligned to 16.
- libgcc: gcc mainline ≈ Dec 1999–2000 sources built with 2.96 at
  -mlong32; EH registry semaphore-guarded (Sony gthr).
- varargs: fetch-then-advance va_arg (newlib/gccinc override).
