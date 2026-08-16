# Codegen levers

Ways to move ee-gcc 2.9x's output without changing what the code means.
Each entry says what you SEE in the diff, what to WRITE, and why.

Reach for these before a fixer flag, and before the permuter. A lever
changes the C; a flag patches the assembly afterwards and is portability
debt. Roughly two thirds of the near-misses closed in this project fell
to a lever, not a flag.

Triage first (`tools/checkfile.py` prints the class):

- **LENGTH** — the C shape is wrong. Fix the shape; no flag will help.
- **REGISTER / OPERANDS** — right instructions, wrong registers. Register
  allocation levers below.
- **SCHEDULING** — right instructions, wrong order. Scheduling levers, or
  a reordering flag once you have proved no source shape reaches it.

---

## Register allocation

**Scope block-local pointers to the block.** Address temporaries declared
once at function scope and reused across several arms of a dispatch
become multi-block pseudos. `local_alloc` only handles pseudos whose
whole live range is inside one basic block, so it skips them;
`global_alloc` then assigns them out of `REG_ALLOC_ORDER` and pushes the
long-lived variables into whatever is left. Symptom: correct length,
nearly every register wrong. Declaring the same temporaries inside each
arm's own `{ }` puts them on the natural `$v1,$a0,$a1,$a2` sequence.
Measured: 71 differing words of 89 → 6, in one edit.

**Declaration order of two `p = obj->field` initialisers.** gcc 2.96
emits the SECOND where it stands and sinks the FIRST to its use. That
decides which pointer keeps a callee-saved register across a call and
which reuses a dying argument register. Worth ~5 words either way.

**`&STRUCT.member` through a local pointer holds the address.** When the
original keeps a member ADDRESS live in a register (`addiu $v0,$s0,28`
then `lw/sw 0($v0)`) and you emit `lw/sw 28($s0)`, naming the member
twice gives the offset form; going through
`pp = &FS.pStream; p = *pp; ... *pp = p+1;` gives the held-address form.
This is a lever, not steering.

**A named local for a double-deref costs the tie-break.** `pCns =
*ppCns` makes gcc reuse the dying argument register; repeating
`(*ppCns)->` and letting CSE invent the temporary puts it in `$v0`.

**`PIN(x,"$2")` + `LAUNDER_V` splits a constant gcc has CSEd** between a
data store and a `switch` case label (e.g. `nSlideY = 48` merged with
`case 0x30:` into one callee-saved register, where retail materialises
them independently).

**One local, two roles.** A C local is one pseudo, so reusing it for two
sequential purposes pins the second value into the first's register.

**Transform parameters in place** when the original does — each result
then shares its argument's callee-saved register.

**Sibling branches that each declare their own local get their own
register.** One shared variable gets one register; C89 block-scoped
declarations in each arm reproduce `$a2` on one arm and `$t0` on the
other.

**The constant-range lever.** Hoist a literal into a long-lived temp to
demote its register-allocation priority.

---

## Loops and induction variables

**Giv-anchor order.** Loop strength reduction anchors the induction
variable on whichever address expression the body forms FIRST. Writing
`s = &arr[i].sub;` before `m = &arr[i];` makes gcc materialise
`%hi/%lo(arr + off)` and reach the base with `addiu reg,v0,-off`. This
is what "phantom symbols at sym+K" in prologues actually are — there is
no missing symbol.

**Address-form vs value-form givs.** A pointer used only as a VALUE (a
call argument) becomes its own accumulator plus a hoisted base; used as
a MEM base it folds into the offset. Worth 6-7 instructions per site.

**A `char *` walk reaches giv anchors no struct-pointer spelling can.**
When the induction variable anchors at an interior member (`p+12`, i.e.
`&vtx[0].nColor`) and neighbours are reached at `+8`/`+19`, no spelling
of `&p->vtx[i+1]` produces that base. `char *q = (char *)&p->vtx[0].nColor`
does — and it frees the counter so gcc reverses it into a `bgez`
down-counter.

**Walked tables want a block-local pointer.** `u_char *q = base + 2 +
i*2;` inside the loop, then `q[0]`/`q[1]`. Writing `base[i*2+2]` puts
the giv at base+3 with -1/0 offsets.

**The goto-loop lever.** When the original branches back to its own TOP
test with an unconditional `b`, write the loop with an explicit `goto`.
`while`, `for`, `for(;;)+break` and `while(1)+break` ALL carry a
`NOTE_INSN_LOOP_BEG`, and jump.c's `duplicate_loop_exit_test()` inverts
them. Only the goto form has no loop note. You lose gcc's
`.p2align 3,,7` loop-top alignment, so spell any pad `SCHED_NOP()`.

**A rotated loop turns loop-invariant motion OFF, and it is reachable
from C.** Enter the loop by jumping to a bottom test:

```c
i = start - nStep;
goto test;
do { i += nStep; ...
test: ; } while (i != END_EXPR);
```

In a rotated loop `scan_loop` sets `maybe_never`, so `move_movables`
refuses to hoist anything `may_trap_p` — and a MEM traps. Only
`load_mems`, which runs AFTER `move_movables`, lifts the loads, and it
lifts them as HImode pseudos. **So `lhu` + `sll`/`sra` where you
expected `lh` is a FINGERPRINT of a rotated loop, not of a cast in the
source.** Everything downstream of the un-hoisted loads stays inside
too, which is why address arithmetic rematerialises each iteration.
Measured: 312 bytes → 264, plain C, no steering.

**Loop direction.** gcc reverses a `for (i = 0; i < N; i++)` whose
counter is dead into a down-counter, and the `li N-1` it creates
schedules LATE. Writing the loop backwards materialises it early and
never matches.

**Put the bound in the loop header, not a local above it** — that is
what creates "load into one register, copy into another".

**Do not out-clever gcc's peeling.** A manually peeled first iteration
gets peeled again.

**`.p2align` only appears for loops gcc RECOGNISES.** goto-loops get
neither unrolling nor alignment; real loop forms with a small constant
trip count get fully unrolled at -O2.

---

## Control flow

**Single exit.** Early returns duplicate the return-value move and block
cross-jumping. Assign on every path and fall out of one `return`.

**Shared epilogue.** "The original has one `return 0` and we have two" is
fixed by making the second path fall through to the first, or `goto` a
shared label.

**`switch` vs `if/else-if`.** gcc sorts the case TESTS but emits the
BODIES in source order — that asymmetry is often the whole layout
difference. Case-label order decides block layout.

**Range tests are order-sensitive.** "x is 32 or 33" must be NESTED IFS;
both `&&` and `||` let gcc fold it to `(x-32) < 2`. Conversely "state is
-1 or 0" only matches as the byte-width wrap `(u8)(nState + 1) < 2`.

**Read the delay slots first.** A non-annulled delay slot always
executes, so whatever gcc puts in one is UNCONDITIONAL in the source.

**"Compute it before the branch" is the branch-likely lever.** When the
original annuls an epilogue restore and gcc emits a plain branch, gcc
had something sinkable; hoist the value above the branch and the annul
appears by itself.

**Float compare polarity.** `if (a > b)` gives `c.lt.s b,a` + `bc1f`;
`c.le.s a,b` + `bc1t` needs the EARLY-EXIT form (`if (a <= b) goto
done;`). And `goto done` folds into the branch where `return` builds a
two-instruction trampoline.

---

## Types, widths and constants

**`long` is 64-bit here.** `(long)` casts give dsll/daddu chains where
plain int gives sll/addu.

**16-bit truncation trap.** `unsigned short v = field;` narrows a later
range test to 16 bits and costs `andi 0xffff` plus `li`+`addu`.
`unsigned int v = (unsigned short)field;` keeps it in int domain.

**`c->s * 4` gives `lh`+`sll`; `c->s << 2` gives `lhu`+`sll`.** Multiply
and shift are NOT interchangeable for matching.

**Read a byte global into an `int` local before comparing** — compared
directly, 2.96 knows the `lbu` result is 0..255 and narrows `slti` to
`sltiu`.

**Split a packed field into named locals.** `p->n >> 16` and
`p->n & 0xffff` inline makes combine narrow the high half into its own
`lhu` at +2 — two loads for one field.

**Struct-copy alignment picks the move idiom.** align 1 → all `lwl/lwr`;
align 4 → `ldl/ldr` for 8-byte chunks plus a `lw/sw` tail; align 8 (the
`union {...; long long ll[2];}` idiom) → `ld/sd`.

**Integer-first addition.** C pointer arithmetic ALWAYS normalises
`int + ptr` to `ptr + int`, so no spelling of pointer arithmetic yields
`addu v1,v1,a1` (offset + base). Write `(T *)((i << k) + (int)base)`.

**`a = b = c = 1.0f` stores c, b, a.** Three separate statements will not
reproduce that order.

**ee-gcc 2.96 truncates every float constant fold toward zero** —
`0.1f` → 0x3dcccccc, `1.0f/3.0f` → 0x3eaaaaaa, all one ulp low. The
original build's compiler did too. If a constant is one ulp off, write
its exact decimal expansion.

**A float constant in the data is usually a literal in the source.** gcc
never sinks an `li.s` into a delay slot, so `return 65535.0f;` and
`return D_004D7F4C;` give different schedules.

**Large constant + pointer vs + symbol.** `ptr_var + 369760` needs
`lui/ori/addu`; the same constant on an array SYMBOL folds into
`%hi/%lo`.

**Named symbols, never numeric casts.** `extern int isInit;` emits the
`lui+addiu` the original has; `(int*)0x0099xxxx` always emits `lui+ori`
and can never match. Add a name to `config/symbol_addrs.txt` instead.

---

## Stores and memory

**Store order is a search, not a deduction.** Several "no permutation
reproduces it" notes turned out to be hand attempts; exhaustive or
hill-climbing search matched functions outright (one needed 504
placements). Use `tools/permute.py`, and NARROW the marked block — a
nine-store block is 362,880 orderings, five stores is 120.

**Reverse the source order of two independent stores.** The scheduler
puts stores back in address order, but the order it materialises their
address/constant operands follows the source — and that fixes register
assignment.

**Field order in the struct is a store-order lever.** A BGRA-in-memory
vertex declared BGRA lets `.r/.g/.b` in natural source order emit the
descending 18/17/16 stores; declared RGBA it cannot.

**Hoisting a constant into a local assigned before a scheduling fence
moves its `li` earlier** without moving the store that uses it. Written
inline, the `li` lands after the first field load.

**Decouple address computation from store order** by hoisting the
addresses into locals.

**Read-all / compute-all / store-all** for multi-field
read-modify-write, or gcc emits load-compute-store triples.

**Load-before-store source order is load-bearing** — gcc will not
reorder a store past a possibly-aliasing load.

**Do not cache a global across a call** — the original re-reads (it
must); caching costs a callee-saved register and a wider frame.

**Read a field back out after the `if`**, do not assign the local inside
it — fixes both the store's source register and gcc's jump threading of
a redundant re-test.

---

## Suspect the C, not the compiler

Several matches in this project were BUGS, not codegen. If the diff
shows any of these, check the source first:

| Symptom | Cause |
|---|---|
| Dead argument register set before a call | Prototype too short. This ABI passes EIGHT args in registers: `$a0-$a3`, `$t0-$t3` |
| `mtc1`/`cvt.s.w` around a call | Missing prototype — the callee defaulted to `int` |
| `lhu` vs `lh` | Field is unsigned |
| `sltu` vs `slt` | Bound is unsigned |
| Two live pointers where you have one | A dropped return value |
| A whole extra argument at a call site | The original really does pass it |

---

## When a lever is not the answer

Some things are genuinely unreachable from C with this compiler, and
they are recorded so nobody re-sweeps them:

- An unfolded `xori reg,reg,0` from `gen_conditional_move` — gcc 2.96
  guards that transform on `op1 != const0_rtx`.
- MIPS `LOAD_EXTEND_OP` makes plain QImode moves `lbu`; a sign-extending
  `lb` for a byte copy has no spelling.
- gcc pads short loops on the R5900 with bare
  `.set noreorder / nop / .set reorder`. A body under ~7 real
  instructions gets padding, so "extra" words in the original may be
  real work in slots gcc filled with nops.

And a warning: a near-miss that TRIAGES as register allocation can still
be a source problem. `--swap-regs` once cut `find_fde` from 20 diffs to
13, which reads as progress and would have permanently masked that the
loop shape was wrong. For vendored upstream code (libc, libgcc), the
first question is *which revision*, never *which flag*.

---

## Hazard-pad flags: which one

Classic gas masks the low bit off both sides when comparing FP registers
(`(op & ~1) == (dest & ~1)`), because with 32-bit FPRs `$fN` and
`$f(N^1)` are the two halves of one double. So:

**Any `li.s`/`mtc1` into an odd FP register followed by a COP1 compute
on its EVEN partner is a `--fp-pair-hazard` site, not an `--mtc1-nop`
site.** No exact-register heuristic finds it. Try `--fp-pair-hazard`
before hand-placing an index-keyed `--mtc1-nop`; it is cheaper and more
principled.

`--barrier-return-store` targets GAS stealing the store into a return
delay slot. When GCC ITSELF filled that slot, `--unfill-gcc-slots` is
the right pass. The two read as interchangeable and are not.

---

## Added from the sound driver (sef/sdv/srs/ssd)

**Two loops sharing a base pointer: spell the SECOND one with
subscripts.** When a tail loop starts where the first loop ended, a
walking `*p = 0; p++;` lets gcc reuse the already-live tail-address
register directly. The subscript form `p[i] = 0;` makes it build its own
induction variable off a *copy* of that register — the `move v1,a2` plus
the extra R5900 pad nop the original has. Closed sefMemZero outright.

**A repeated division by the same constant must be a named local.**
`x / 60 / 60` written inline is folded to `x / 3600` and one `divu`
disappears. Writing `m = x / 60;` and then using `m / 60` and `m % 60`
keeps both divisions and lets them share one `divu`. (Symptom: `li 3600`
where the original has several `li 60`s — gcc CSEs the `divu`s but never
the constants, so a count of surplus `li K`s tells you how many division
SITES the source had.)

**An early `return CONST` hoists the constant to the top of the
function.** When the original materialises a return value in its first
few instructions and you materialise it at the end, the guard arm is a
separate `return 1;`, not a fall-through to a shared expression that
happens to evaluate to 1. Pairs with the `slt`/`sltu` row: a hit counter
compared `< 1` only gives `sltiu` when the counter is `unsigned`.

**Declare a float literal and its destination pointer INSIDE the arm
that uses them, not pinned above the call.** `register float k
__asm__("$f0") = ...;` above a call is silently dropped when the callee
returns in `$f0`; a plain local inside the `if` body gets the `lwc1` and
the address `addiu` scheduled into the call's and the branch's delay
slots, which is where the original has them.

**Prototype mismatch between two call sites of the same function is
real.** `sdvPlaySound` is called with three arguments from one site and
one from another, and `sefCnvEtEffectNo` with two and one. Widen the
definition (dead parameters cost nothing) and give the narrow call site
an aliased view: `extern int f1(int a) __asm__("f");`. Do not "fix" the
narrow site by passing extra arguments.

**Exhaustive permutation pays on small store blocks.** Four independent
header stores in sefFreeSchedulerCf come out in neither source nor
address order; 24 orderings is a two-minute script and exactly two of
them reproduce the original.

### Swept and unreachable here

- **gcc cross-jumps identical `jr ra / move v0,aN` epilogues that the
  original duplicates.** In sefCnvEtEffectNo three bands each have their
  own fall-through epilogue; no arrangement of `return` vs `goto out`,
  in either arm order, stops the merge, and dropping the `goto` turns
  the arms into `movz`/`movn` instead.
- **`(unsigned short)(x - K)` has three spellings and none gives
  `addiu -K` + `andi 0xffff` in the block the original puts it in.**
  Inline, gcc folds the `andi` away; as an `unsigned short` local it
  builds the constant with `li 0xff69` + `addu`; as an int local
  narrowed at the test it emits both but sinks them past the preceding
  branch (a bare `__asm__ __volatile__("")` barrier pulls them back and
  leaves only a register-naming difference).
- **A `(short)` cast hoisted into its own local becomes a second `lh`
  load.** Left inline against a value already loaded with `lhu`, it
  stays the `sll`/`sra` pair the original has.

---

## From the Char/Check/ACT/sub/Hit run

**Alignment variants of ONE struct tag, never two struct types.** When
two members of the same struct must be copied with different move idioms
(`ld/sd` from one, `ldl/ldr` to the other) the alignments have to differ.
Spelling the 4-aligned one as a *second, identically-shaped struct* puts
it in a different gcc alias set, and gcc will then hoist the read of a
member above the store that fills it -- wrong code, not just a mismatch.
Declare the record once and typedef it twice:

```c
struct EVECS { float x, y, z, w; };
typedef struct EVECS __attribute__((aligned(8))) EVEC;  /* ld/sd   */
typedef struct EVECS EVEC4;                             /* ldl/ldr */
```

Variants share `TYPE_MAIN_VARIANT`, so they share the alias set.
**Corollary: member-level `__attribute__((aligned(8)))` does not work.**
ee-gcc 2.9x takes block-move alignment from `TYPE_ALIGN`, not
`DECL_ALIGN`; moving the attribute off the typedef onto each declaration
silently turns every `ld/sd` vector copy in the file into `ldl/ldr`.

**Switch node COUNT picks the decision-tree root.** `balance_case_nodes`
has a special case for exactly three case nodes -- it splits at the
middle, so `case 1/2/3` roots the tree at 2 (`beq 2`, then `slti 3`).
A fourth node roots it at 1 (`beq 1`, `slti 2` -> default, `beq 2`,
`beql 3`). If the original's dispatch starts at the lowest label, the
source has a case you have not written -- usually an empty idle state
(`case 0: break;`). This is invisible in the diff as anything but
"the whole dispatch is shaped wrong".

**Let CSE invent the FP temporaries.** Naming `cur`/`dst`/`v` as locals
in a float easing routine rotated every FP register and made the product
reuse the argument register `$f12`. Leaving `*pCur`/`*pDst` as memory
reads, with only the difference in a local, made gcc create the
temporaries in the original's order -- and the product then reused the
dead `0.0f` register. Two locals fewer, 18 diffs to zero.

**Sequence a call into its own local when its result feeds an argument
list.** `Check_Angle(Get_Angle(a, b), c - half, c + half)` makes gcc
compute `half` BEFORE the inner call, so `half` needs a callee-saved
`$f20` and the frame changes. `angle = Get_Angle(a, b);` on its own line
computes `half` after the call, in the caller-saved `$f13` that then
becomes the argument. Worth a whole instruction and four register names.

**A store of a constant next to a conditional wants a ternary.**
`n = 2; if (!flag) n = 1;` sinks the `li 2` above the test and emits its
own `sw`. `n = flag ? 2 : 1;` gives one `sw` fed by a `li` in the branch
delay slot.

**`^ 1`, not `!` or `== 0`.** `xori v0,v0,1` followed by a separate
`sltu v0,zero,v0` only appears when the negation is spelled as an XOR;
`!x` and `x == 0` both fold to one `sltiu v0,v0,1`.

**Runs of same-block stores come out in REVERSE source order.** Three
independent field stores closing a UI panel had to be written
`nWidth / nMode / bFlags` to be emitted `nWidth`, `nMode`... no: to be
emitted in the original's `nWidth`, `bFlags`, `nMode` order. When a short
run of stores is the only difference, try the reversal before anything
else, then permute exhaustively -- it is a six-case script.

**An unaligned struct copy is order-sensitive against float work.** In
Check_Undu, writing the 16-byte start-point copy before the direction
vector made gcc emit the whole `ldl/ldr/sdl/sdr` block ahead of the float
loads and rotated every callee-saved register (44 diffs). Written after
the direction vector: exact.

**Eight-argument calls are normal here.** `$a0-$a3` plus `$t0-$t3`.
`Check_Undu` takes seven. Two of them are stored with `sd`, so those
fields are 64-bit even though the arguments are plain `int`.

### Swept and unreachable here

- **Which of two loop-invariant constants gets the lower callee-saved
  register.** In ACT_initSequence gcc hoists `32` and an
  `ACT_updateDefault` address out of the loop into `$s4/$s5` and assigns
  them the opposite way round to the original. The two fields' source
  positions swap the STORE order and the register mapping *together*, so
  neither ordering reaches the original. Hoisting either value into a
  plain local, or pinning both, is worse. Permuter territory.
- **Recomputing `lui %hi(sym)` at the top of each of several loops over
  the same array.** ACT_pauseUpdate has three loops over `actor[]`; the
  original rematerialises the `%hi` each time, gcc hoists it into a spare
  register once. Scoping the pointers per loop makes it worse, not
  better.
