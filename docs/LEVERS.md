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

**A void-result call's declared RETURN TYPE is a register-allocation
lever.** Declared `void`, gcc kept an unrelated read-modify-write in
`$v0`; declared `int`, it moved to `$v1` and the function matched — same
call, same arguments, no extra instruction. Check the prototype before
reaching for a `PIN` when a near-miss is pure `$v0`/`$v1` naming around a
call.

**A constant that must cross a call has to be a local defined before
it.** gcc 2.96 does NOT constant-propagate a local whose live range
spans a call.

**Advance a pointer local immediately after storing it** (`t->pp = p;
p += 3;`) to kill the CSE that suppresses retail's reload.

**A separate loop-counter variable per loop.** Reusing one `i` across
two loops forces it into a callee-saved register and swaps the
counter/pointer registers.

**A byte field read for both a test and arithmetic wants the local
INSIDE the arm** — `if (t->b) { n = t->b; ... }` recovers retail's `move`
AND its `.p2align` pad: two words of LENGTH, not just registers.

**Which of two `x | c` values reuses the shared temp's register is
decided by which variable you SEED with the temp**: `gt = n; eop = gt|C1;
gt |= C2;`.

**PIN the first parameter to sink its incoming-argument copy to the
end** — otherwise the whole prologue's `sd`/`move` interleave comes out
rotated by one. Fixed three functions.

**An empty asm is the only way to keep a combine-folded compare temp
alive** — a bare `PIN` on it is silently ignored — **and it buys the
register at the price of the schedule** (9-14 words for this gcc).

**-fschedule-insns runs BEFORE register allocation, so the pre-RA
scheduler decides which hard-register ARGUMENT COPY is live at a
compare, and that decides the compare temp's register.** When gcc hoists
`move $a1,arg` above the compare, `$a1` is taken and local_alloc gives
the combine-folded temp the next free number. Diagnose it by blocking
the register you were given with a PIN'd dummy: if the temp moves to the
NEXT number instead of the one you want, the register you want is not
free, and no source shape will free it -- the choice was made by the
scheduler's tie-break between two equal-priority argument moves, which
is not reachable from C. `sceVif1PkRefLoadImage` cost ~40 source shapes
and 25 flags proving this before it was closed with a ranged
`--swap-regs`.

**`--swap-regs` now takes an instruction RANGE**: `FUNC:A-B:LO-HI`, or
several windows `FUNC:A-B:91,98` (bare index = single instruction),
counted like `--swap-adjacent` over gcc's asm. That is what a register
tie-break confined to one window needs when the same registers are
ordinary ABI arguments everywhere else in the function, which is where
the old whole-function form was useless.

**"Registers AND order in the same two words" = rename then FORCE-swap.**
Neither fixer alone reaches it. Use the ranged `--swap-regs` to fix the
registers, then `--swap-adjacent FUNC:N!` to fix the order -- the `!` is
required because after the rename the pair usually looks dependent to
`swap_ok`. Check that the FINAL code is correct (it is, when the
original's bytes are what comes out) and let `tools/audit_swaps.py`
confirm the reorder. `sceSifInitIopHeap` fell to exactly this: gcc's
`lui $3,%hi` / `move $2,$0` / `sw $0,%lo($3)` versus the original's
`lui $2` / `sw ...($2)` / `move $2,$0`.

**Fixer SITE INDICES are not diff indices.** `--swap-adjacent`,
`--rotate` and friends count instructions in the POST-PROCESSED ASM,
which is before gas steals an instruction into a delay slot;
`--swap-regs` counts in gcc's OWN asm, before every fix_cc_asm pass.
Both can be several lower than the position you read off the
disassembly, and a sweep centred on the diff index simply finds nothing.
`sceMcRename` sat parked for a whole session because sites 54..58 were
swept from the diff and the answer was 53. Dump the real numbering:

```python
import sys; sys.path.insert(0, "tools"); import fix_cc_asm as F
lines = open("out.s").read().split("\n")
own = F.function_at(lines); idx = 0; prev = None
for i, l in enumerate(lines):
    if own[i] != prev: idx = 0; prev = own[i]
    if F.RE_INSN.match(l): print(own[i], idx, l.strip()); idx += 1
```

**One local, two roles.** A C local is one pseudo, so reusing it for two
sequential purposes pins the second value into the first's register.
This cuts BOTH ways and the direction is worth testing: reusing the
function's actor pointer for a later, unrelated pointer was the whole
difference between a match and 60 diffs in `updateCursorMode2`, while
reusing ONE `char *` local for two format strings cost 16 diffs in
`updateCursor` because the shared pseudo's longer live range pushed it to
the end of the callee-saved order and shifted every other assignment.

**A constant used on both sides of a call gets a callee-saved register.**
Two tests against the same literal with a call between them become one
pseudo that crosses the call, so `global_alloc` gives it $s2 and the
whole frame grows; the original rematerialises it with a second `li`.
No source spelling found so far separates them -- swapping the arms,
comparing a different equivalent constant, block-scoping the locals and
LAUNDER on the loaded value all leave the CSE intact. Recorded because it
is the last word standing between `tskUmnSimulationList` and a match.

**Assigning a field and a local the same constant: two statements, not a
chain.** `p->nMode = 2; n = 2;` materialises the constant TWICE, which is
what the original does when the local then feeds a range test.
`n = 2; p->nMode = n;` shares one register and comes out a word short.

**Let CSE invent the FP temporaries.** Naming `cur`/`dst`/`v` rotates
every FP register; leaving them as memory reads matches.

**Sequence a call into its own local when its result feeds an argument
list** — otherwise gcc computes the other arguments first and they need
callee-saved FP registers.

**Transform parameters in place** when the original does — each result
then shares its argument's callee-saved register.

**Sibling branches that each declare their own local get their own
register.** One shared variable gets one register; C89 block-scoped
declarations in each arm reproduce `$a2` on one arm and `$t0` on the
other.

**`LAUNDER` the pointer, not the value,** to hold a run of stores through
it in source order — it also stops gcc duplicating one store into a
branch delay slot.

**A `PIN` can be actively HARMFUL.** Pinning the *last* long-lived value
removes the temp retail loads through. Pin the first N and let the next
register fall out.

**Drop the named local and let CSE invent the temporary.** In
`nmlPacketTextureTrans` a `nOfs = *(int *)(pTex + 0x6C);` local put the
pointer in `$v0` and the offset in `$v1`; writing the field expression
twice and letting CSE make the temp reversed them -- and that one change
also fixed the prologue's `sd` order and which instruction filled the
`blez` delay slot. Same axis as the double-deref entry above, and it is
worth trying in BOTH directions before reaching for a flag.

**Delete steering before adding any.** One function matched tonight
purely by removing two stale `LAUNDER`s — the scheduler then emitted the
retail order by itself.

**Raise register pressure early.** Reading a global into a local before
address arithmetic moved a pointer off `$a0`.

**The constant-range lever.** Hoist a literal into a long-lived temp to
demote its register-allocation priority.

**The operand order of a commutative `addu` is observable, and it follows
the source.** `*(int *)((char *)p + ofs)` emits `addu v0,p,ofs`;
`*(int *)(ofs + (int)p)` emits `addu v0,ofs,p`. The integer cast is
load-bearing -- with pointer arithmetic gcc canonicalises the pointer to
the first operand however you write it, so the offset has to be added as
a plain integer. This was the LAST word between GameDefocusSet and a
match.

**A redundant `andi 0xffff` in the original means an explicit `(u_short)`
cast in the source.** When retail truncates a value a preceding `lhu`
already zero-extended, the cast is really there: without it gcc proves
the value fits and drops the word, and the function comes out one
instruction SHORT. Same for `andi 0xff` after an `lbu`. (RSRC_alloc.)

**`LAUNDER2` over `int` copies defeats loop-invariant motion of gp
loads** — the pseudo is then set twice in the loop. As `unsigned short`
locals the same trick costs two extension moves.

**LAUNDER2 fences the ORDER of two values against the EE scheduler
where a single LAUNDER on either one does not.** In RSRC_alloc the
scheduler issued a `(u_short)` truncation ahead of the `+1` increment
that shared its source register; with the truncation first, the
increment still needed the untruncated value, so the truncation could
not overwrite the dead loaded register in place and every role shifted.
`LAUNDER(x)` alone went from 6 diffs to 54; `LAUNDER2(next, count)`
matched. Reach for LAUNDER2 whenever the diff is "these two independent
computations are in the wrong order".

**LAUNDER_V between the two halves of a split shift lets an unrelated
store schedule between them.** `n = (x + 15) >> 4; LAUNDER_V(n);
n <<= 4;` reproduces retail's `srl` / `sw` / `sll` interleave; written as
one expression the pair stays adjacent. (RSRC_alloc.)

**Transform a parameter IN PLACE inside the guarded region.** `nSize =
(nSize + 63) & ~63;` after the guard gives retail's `move s0,a0` in the
call's delay slot plus a separate `addiu`/`and` pair; a fresh
`nAligned` local lets gcc merge the argument copy with the rounding and
steal the delay slot. Took GameResourceAlloc 31 diffs -> 6.
(This is the same "one local, two roles" family, applied to a
parameter.)

**Which arm is written FIRST decides the branch polarity, and it is
readable off the original.** A conditional branch that jumps FORWARD
over a block means that block is the fall-through arm, i.e. the source
tests the OTHER condition. Writing `if (n >= cap) recycle; else
append;` rather than `if (n < cap) append; else recycle;` was worth
three words in RSRC_alloc -- gcc will not lay the arms out the other
way round for you.

**Read both fields into locals before any store when the original loads
them together.** A load written after an intervening store to the same
object cannot be hoisted (gcc must assume the store aliases), so the
schedule is fixed by source order, not by the scheduler.
(GameResourceAlloc.)

**A scratchpad A+D block wants inline absolute casts, not a pointer.**
`((GSENTRY *)0x70000010)->lData = ...` per entry is the form that
matches: each `0x700000X0` gets its own `lui`+`ori` in its own
caller-saved register, live only across that entry's two stores. One
reassigned `GSENTRY *` local is ONE pseudo, so every entry reuses the
same register and the whole block's schedule is wrong; eight
function-scope pointers all go live at once and push the data constants
into `$s0-$s6`. Confirmed by the five matching `nmlFilter*` builders.

---

## Loops and induction variables

**Giv-anchor order.** Loop strength reduction anchors the induction
variable on whichever address expression the body forms FIRST. Writing
`s = &arr[i].sub;` before `m = &arr[i];` makes gcc materialise
`%hi/%lo(arr + off)` and reach the base with `addiu reg,v0,-off`. This
is what "phantom symbols at sym+K" in prologues actually are — there is
no missing symbol.

**An LSR-built giv's init lands in the loop PREHEADER; a source-walked
pointer's copy lands in the ENTRY BLOCK.** `pTbl[i]` makes loop.c create
the giv and emit its initial value after the loop's guard branch;
`p = pTbl; ... *p++` puts the copy in the function's first block. That
changes two things at once: where the callee-save of that register
appears in the prologue, and -- because the pseudo's live range now
starts at the guard instead of at entry -- its `global_alloc` priority,
so it can swap registers with an unrelated whole-loop value. A `PIN` can
force the register but NOT the placement; only the giv does both.
(subTreeLineDraw: `pType[i]` vs `*pt++` was the whole function.)

**A block-local pointer declared INSIDE the loop body is what makes gcc
keep the base register on the loop's own biv.** `for (...) { OUT *q =
&p->out[i]; q->a = ...; f(q); }` gives one address giv plus one
value-form giv for the call; writing `p->out[i].a` throughout leaves the
base fixed and gcc invents a separate giv for each PAIR of stores --
six extra words in subLine2_DrawType_1. Same edit, opposite direction
from the "scope block-local pointers to the block" register lever, and
it is the first thing to try on any "too many induction variables" diff.

**Address-form vs value-form givs.** A pointer used only as a VALUE (a
call argument) becomes its own accumulator plus a hoisted base; used as
a MEM base it folds into the offset. Worth 6-7 instructions per site.

**A `char *` walk reaches giv anchors no struct-pointer spelling can.**
When the induction variable anchors at an interior member (`p+12`, i.e.
`&vtx[0].nColor`) and neighbours are reached at `+8`/`+19`, no spelling
of `&p->vtx[i+1]` produces that base. `char *q = (char *)&p->vtx[0].nColor`
does — and it frees the counter so gcc reverses it into a `bgez`
down-counter.

**Indexed vs walked table reads.** `pOfs[i]` makes LSR build a giv whose
init is a separate pseudo, giving the `addiu`+`move` pair; `*pOfs++`
folds them and is TWO WORDS SHORTER. The mirror image of the block-local
pointer entry — a third instance of this same axis, so always read the
original's address arithmetic first.

**Direct member access vs a struct pointer decides `addu` operand
order.** `pEnt = &arr[i]; pEnt->f` gives `addu rd,off,base`; `arr[i].f`
gives `addu rd,base,off`. No spelling of the pointer form reaches the
latter.

**The order of hoisted loop INVARIANTS is the source order of the
statements that use them.** Writing a packet block in plain
`p[0]..p[9]` order matched, while hoisting one element to second place
moved its instruction seven slots and nothing else.

**Two views of one record.** A walking pointer into a BIASED sub-view of
an array element, alongside a byte offset added to the base, is a real
source idiom — four matches in the sound driver needed it. The bias
constant in the field offsets tells you where the sub-view starts; the
stride stays the full record, so pad the view struct to it.

**An LSR-built giv's init lands in the loop PREHEADER; a source-walked
pointer's copy lands in the ENTRY BLOCK.** `pType[i]` vs `*pt++` changes
TWO things at once: where the callee-save appears in the prologue, and
the pseudo's `global_alloc` priority (its live range starts at the guard
rather than at entry), so it can swap registers with an unrelated
whole-loop pointer. A `PIN` forces the register but NOT the placement —
only the giv form does both.

**A block-local pointer declared inside the loop BODY keeps the base on
the loop's own biv.** `for (...) { REC *q = &p->out[i]; ... }` — writing
`p->out[i].member` throughout leaves the base fixed and gcc invents a
separate giv per PAIR of stores.

**Let LSR build the giv.** `arr[i].field` yields `%hi/%lo(arr)` plus a
separate `addiu +off` for the giv's initial value; a hand-walked pointer
folds the offset into the `%lo` and comes out ONE WORD SHORTER — and
that word is what makes the loop-top `.p2align` pad appear. Took one
function 37 diffs -> match. Note this is the opposite direction from the
entry below; read the original's address arithmetic before choosing.

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

**A compound loop condition rotates the loop and turns LICM off.**
`while (r >= 0 && i < 256)` rematerialises a shared `li 10` divisor
three times; moving the second test to a `break` inside keeps it
hoisted. Same mechanism as the rotated-loop entry above, reached
accidentally.

**A deferred store wants its value computed with its siblings.** In an
`n%10` / `n/=10` chain, a store deferred past unrelated stores must have
its digit computed alongside the others, or its divisor's `li 10`
materialises after the character constants and every `$t4..$t7` shifts.

**Loop direction.** gcc reverses a `for (i = 0; i < N; i++)` whose
counter is dead into a down-counter, and the `li N-1` it creates
schedules LATE. Writing the loop backwards materialises it early and
never matches.

**`for (i = 0, p = <invariant>; ...)`** puts the counter's zero AHEAD of
the invariant pointer in the loop preamble; assigning `p` on its own
line before the loop emits them the other way round.

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

**A redundant `else` arm changes block layout.** `if (!p) { q = 0; } else
{ X }` with `q` already 0 moves X after the join and reaches it with
`bnez`+`b`. Worth ten words in one case.

**`goto` the default arm** for a shared return value.

**Retail entry blocks hoist member ADDRESSES above a state dispatch.**
When a `switch (w->nState)` prologue contains `addiu sN,work,K`, the
source had a member pointer local declared before the switch. One edit
took a function 61 diffs -> 11.

**Shared epilogue.** "The original has one `return 0` and we have two" is
fixed by making the second path fall through to the first, or `goto` a
shared label.

**Write a `switch` by OMITTING the dead cases.** gcc spans min..max and
fills the holes with the default, so six labels reproduce a 16-entry
retail jump table.

**Scope a pointer local to the switch ARM that uses it.** A
function-wide `T *p` shared by several arms is one long live range and
the allocator never gives any arm the original's register. Note this is
per-arm: the same rewrite in a second arm did nothing.

**A dispatch whose arms are laid out AFTER the join is a `switch`, not
an if/else-if chain** — and **literal case labels are what keep LICM
alive**. Naming the case values as `int` locals moved their `li` above
the loop guard and stopped six float constants being hoisted into
`$f20-$f25`.

**Switch node COUNT picks the decision-tree root.** gcc's
`balance_case_nodes` special-cases exactly three nodes and splits at the
middle. A dispatch that starts at the LOWEST label means the source has
a case you have not written — usually an empty `case 0: break;`.

**A `beqz`/`bltz`/`slti 3` triple is a SWITCH decision tree**, not an
if-chain.

**`do {} while()` keeps the loop note** where a hand-rotated `goto test`
loses it — the difference between a bound constant hoisted into `$s3`
and rematerialised every iteration. (The goto-loop lever cuts both ways;
see above.)

**A dispatch whose arms are laid out AFTER the join is a `switch`.**
`beq v,K1,A / beq v,K2,B / b JOIN` followed by the A and B blocks is the
switch layout; an `if/else-if` chain inlines the first arm and reaches
the second with a second test after it. Worth checking before assuming a
register or scheduling problem -- in subPosSet it was 120 diffs.

**Literal case labels are what let LICM hoist a loop's constants.**
Comparing against `int` locals initialised to the case values keeps them
in callee-saved registers, but their `li` is emitted where the
declaration is -- ABOVE the loop's guard branch. Spelled as literals,
loop.c hoists the `li` into the preheader instead, and (in the same
pass) hoists every float constant in the body into $f20+ as well. If a
loop's FP constants are being rematerialised in each arm, look for a
nearby named integer constant that has turned LICM off.

**`switch` vs `if/else-if`.** gcc sorts the case TESTS but emits the
BODIES in source order — that asymmetry is often the whole layout
difference. Case-label order decides block layout.

**Range tests are order-sensitive.** "x is 32 or 33" must be NESTED IFS;
both `&&` and `||` let gcc fold it to `(x-32) < 2`. Conversely "state is
-1 or 0" only matches as the byte-width wrap `(u8)(nState + 1) < 2`.

**Guarded arithmetic belongs INSIDE its `if`.** gcc hoists it above the
branch itself and gets the original's register roles; writing it above
the guard by hand does not. One function went 10 diffs -> match.

**Get the frame size right FIRST.** Until the stack frame matches, every
other diff is noise — one buffer was 6 quadwords though the code only
uploads 5, and nothing else could be judged until the frame was 144.

**Arm order reads off the source condition** — a `bnezl` past a call
means that call is the THEN arm.

**A VU0/MMI-only leaf needs `.set noreorder` around its asm.** Without
it gas pulls the block's LAST instruction into the `jr ra` delay slot
and the trailing pad nop vanishes -- the function comes out one word
short. Wrapping the body in `.set noreorder` ... `.set reorder` restores
both. Four `nml` VU leaves fell to this in one edit each.

**Naming hard registers inside an asm block is what pushes its operands
off `$a0-$a2`.** `_WeightToGlobalPlaceVec` uses `$v0-$a3`/`$t0-$t1` by
name for a pextlw/pcpyld transpose; declaring exactly those as clobbers
makes gcc emit the original's three `move` instructions into `$t2-$t4`.

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

**`while ((*p = *q) != 0) { q++; p++; }`** — the assignment EXPRESSION
gives one `lbu` plus the `sll 24`/`bnez` QImode test. A named `char c`
emits a second `lb`; an `unsigned char c` drops the `sll`.

**`li at,0xffff` + `addu` means the source literally adds `0xFFFF`**,
not `-1` (which folds to a single `addiu`).

**A hardware/scratchpad store's addressing mode is a source choice.**
Plain `*(char *)0x70000021 = 1` gives the absolute `lui at` form; a
`volatile` pointer local gives the `lui`+`ori` register-base form. Worth
~15 words if you pick wrong.

**Volatile stores are scheduling barriers a load cannot cross** — read a
field into a local BEFORE a volatile-store block if the original loads
it early.

**A block-scope array with an INITIALISER is a TEMP slot in gcc 2.9x.**
It is freed at the end of its block and REUSED by the next same-sized
array in a LATER block — which silently shrinks the frame and moves
every `sd`/`ld` offset after it. Declaring the second array one entry
larger makes it too big for the freed slot. Measured: **111 diffs -> 33
in one edit.**

**An array INITIALISER becomes a constant-pool copy; the same array
filled by ASSIGNMENTS is inlined.** `int nAdd[2] = {1,-1}` gives
retail's `ldl/ldr` + `sdl/sdr`; two `nAdd[k] = ...` statements give two
`sw`.

**A nested struct whose last field is 4 bytes but which contains an
8-byte field is rounded up to a multiple of 8**, pushing everything
after it. Spell the block out inline to land on retail's offsets.

**Chain a second struct copy off the first.** `a = src; b = src;` where
`a` and `b` are in the same alias set as `src` RELOADS src for the second
copy -- the first copy's stores kill it. `a = src; b = a;` lets cse
forward the just-stored value, which is two loads and four stores instead
of four and four. (subTreeLineDraw_type_2's segment pairs.)

**A `char`/`unsigned char` store aliases every load in gcc's model.**
When a float re-read straddles a byte store for no visible reason, that
is why -- and it is a scheduling barrier you can place deliberately:
sub2ObjectLampDraw needs the sprite width and height written BEFORE the
alpha byte and the position AFTER it, which is what splits the tree
scale into two loads.

**A sum stored untruncated but tested as a narrower type wants the local
to keep the WIDE type and the tests to carry the cast.** `p->nPhase`
(`unsigned short`) `= n; if ((short)n < 0) ... else if ((short)n >= 129)`
gives one `sll`/`sra` feeding both branches and stores the raw `addu`
result. Declaring the local `short` stores the truncated value and costs
three words.

**Check the struct's own ALIGNMENT when a `char pad[]` stops working.**
Giving one struct a leading `int *` rounded its size 0x12 -> 0x14 and
shifted every later member, surfacing as IMMEDIATE diffs in three
already-matching functions.

**`p->field--` on an `unsigned short` compiles to `li 0xffff` + `addu`.**
Read into an `int` first for retail's `addiu -1`.

**A member address folds into the symbol unless you go through a pointer
local.** `TBL[n].field` gives `lui/addiu` of `sym+8` into a pseudo;
`REC *p = &TBL[n]; p->field` keeps the bare symbol in `$at` and the 8 as
the load offset.

**`long` is 64-bit here.** `(long)` casts give dsll/daddu chains where
plain int gives sll/addu.

**16-bit truncation trap.** `unsigned short v = field;` narrows a later
range test to 16 bits and costs `andi 0xffff` plus `li`+`addu`.
`unsigned int v = (unsigned short)field;` keeps it in int domain.

**`^ 1`, not `!` or `== 0`,** for the `xori`+`sltu` pair.

**`c->s * 4` gives `lh`+`sll`; `c->s << 2` gives `lhu`+`sll`.** Multiply
and shift are NOT interchangeable for matching.

**A `union { TI q; u_int w[4]; }` is not optional for a quadword
buffer you then patch a word of.** With `u_int aTag[4]` plus
`*(TI *)aTag = ...`, gcc 2.96 does not see the alias and hoists the
`lw 4(sp)` ABOVE the `sq` that fills the buffer. That is a MISCOMPILE,
not a mismatch -- the union both fixes it and removes 12 diffs.

**Read a hardware halfword at EACH USE rather than caching it in an
`int`** to get retail's `lhu` + `andi 0xffff` pair. CSE still folds the
loads, but each equality test against an int constant needs the value
widened and the widening gets its own register. Caching fuses the two
into one `lhu` (-2 words); caching it after an intervening store is
worse still.

**Read a byte global into an `int` local before comparing** — compared
directly, 2.96 knows the `lbu` result is 0..255 and narrows `slti` to
`sltiu`.

**Split a packed field into named locals.** `p->n >> 16` and
`p->n & 0xffff` inline makes combine narrow the high half into its own
`lhu` at +2 — two loads for one field.

**Struct-copy alignment picks the move idiom.** align 1 → all `lwl/lwr`;
align 4 → `ldl/ldr` for 8-byte chunks plus a `lw/sw` tail; align 8 (the
`union {...; long long ll[2];}` idiom) → `ld/sd`.

**`t->shortfield = t->shortfield + 1` keeps the value RAW** —
`lhu`/`addiu`/`sh`, with the extension deferred to each use. A `short`
local extends at the definition instead. Which side of the `addiu` the
`sll/sra` sits on tells you which the source used. Same for `signed
char` read-modify-write chains: use the member, not a local.

**A short-truncated sum of an `int` field needs its own `int` local**, or
combine narrows the load to `lhu`. Worth 4 words.

**`(T *)(n + (u_int)p)` is the only way to get `addu rd,<int>,<ptr>`.**

**Grouping decides whether two constants fold.** `BIG | 0x24020000 |
(field << 5)` left-associates, so gcc folds `BIG | 0x24020000` into one
constant and the function comes out a word SHORT. `(0x24020000 |
(field << 5)) | BIG` ORs the small constant with the non-constant first
and keeps both materialisations. Symptom: LENGTH, one or two words, with
an `ori` immediate that is the two constants merged.

**Integer-first addition.** C pointer arithmetic ALWAYS normalises
`int + ptr` to `ptr + int`, so no spelling of pointer arithmetic yields
`addu v1,v1,a1` (offset + base). Write `(T *)((i << k) + (int)base)`.

**`a = b = c = 1.0f` stores c, b, a.** Three separate statements will not
reproduce that order.

**A general float constant is a `li.s` MACRO, and gas's own -G decides
what it expands to.** The compiler emits `li.s $f1,<decimal>` for every
SFmode constant. Whether that becomes inline `lui`/`ori`/`mtc1` or a
`.lit4` pool entry loaded through `$gp` is decided by the ASSEMBLER's
small-data threshold, which is independent of the -G passed to gcc. At
gas's default -G8 a two-halfword constant is pooled and costs ONE word;
the original build's inline form costs THREE. Symptom: a `lwc1
$fN,off($gp)` where the original has `lui $at`/`ori $at`/`mtc1`, and a
function two words short. The fix is `FILE_ASFLAGS_OVERRIDE[file] =
"-G0"` (set it with `tools/set_flags.py --asflags`, and declare it to the
post-processor with `--as-g0` so the COP1 hazard pads come back). A
constant that needs only a `lui` -- 1.0f, 7.5f, 3.0f, 0.5f -- inlines at
either threshold, so a file can look fine until its first awkward
constant. -G0 is a WHOLE-FILE switch and it also stops gas resolving
`lw $reg,extern_sym` through $gp, so any function in the same file that
reaches a 4-byte global that way breaks: give the float-constant function
its own translation unit rather than dragging them down.

**And the decimal round-trip truncates TWICE.** gcc prints the constant
into the `li.s` as a decimal, and gas parses it back. Both conversions
fold toward zero, so `0.001f` leaves the compiler as
`9.99999931082129478455e-4` and arrives one ulp low (0x3A83126E, not
0x3A83126F). Writing the exact decimal expansion does NOT fix this one --
it is the printing that loses the bit. Nudge the source constant UP until
gcc prints the decimal for the value you want (`0.0010000001f` here) and
check the `li.s` line in `cc -S` output, not the source.

**ee-gcc 2.96 truncates every float constant fold toward zero** —
`0.1f` → 0x3dcccccc, `1.0f/3.0f` → 0x3eaaaaaa, all one ulp low. The
original build's compiler did too. If a constant is one ulp off, write
its exact decimal expansion.

**A float constant in the data is usually a literal in the source.** gcc
never sinks an `li.s` into a delay slot, so `return 65535.0f;` and
`return D_004D7F4C;` give different schedules.

**`extern T sym[];` vs `extern T sym[4];`.** An INCOMPLETE extern array
cannot go in small data, so its address costs `lui`+`addiu` instead of
`addiu reg,gp,off`. Scalars get small-data for free; only arrays bite.

**Naming a global symbol twice in one address expression costs an
instruction.** `*((char *)&G + G.field)` makes gcc rebuild `%hi/%lo` for
the second reference; a block-local pointer to the symbol gives the
retail `addiu reg,base,%lo` + `addu` pair. Diagnoses as LENGTH, reads as
register allocation.

**Alias choice decides displacement.** Two symbols naming the same
address generate different code — pick the one that keeps the offset in
the load displacement, since that preserves the shared hoisted `lui`.

**A separate local for `&ARRAY[-K]` folds the constant into `%lo` and
LOSES a word.** A negative constant offset must stay inside the
subscript.

**Struct stride as its two halves.** `(i << 10) + i*80`, not one 1104
multiply — true even outside a loop, so it is a source spelling.

**Large constant + pointer vs + symbol.** `ptr_var + 369760` needs
`lui/ori/addu`; the same constant on an array SYMBOL folds into
`%hi/%lo`.

**Named symbols, never numeric casts.** `extern int isInit;` emits the
`lui+addiu` the original has; `(int*)0x0099xxxx` always emits `lui+ori`
and can never match. Add a name to `config/symbol_addrs.txt` instead.

---

## Stores and memory

**A pointer that is live across a call in ONE branch wants that branch to
own it.** Writing the loop's store through the global by name
(`cursor.pSel = p`) instead of through the enclosing block's pointer is
what keeps that pointer a single-block quantity in a caller-saved
register and gives the loop its own callee-saved copy -- the `move
$s2,$a2` in the preheader. An explicit `q = p;` does not work: regalloc
coalesces the copy away. Measured: 164 words to 166, the last two.

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

**gcc DELETES the first of two stores to the same member**, even with a
non-aliasing load between them. Only a `volatile` store keeps both.

**Chain a second struct copy off the first** (`a = src; b = a;`) —
copying `src` twice reloads it, because the first copy's stores are in
the same alias set.

**A char store aliases every load**, which makes it a PLACEABLE
scheduling barrier.

**A store through a differently-typed pointer kills a struct member's
CSE across it.**

**Two `return` arms with identical tails get cross-jumped** — which
looks like a "length short" bug. Permuting one arm's store order breaks
the merge.

**Load-before-store source order is load-bearing** — gcc will not
reorder a store past a possibly-aliasing load.

**Do not cache a global across a call** — the original re-reads (it
must); caching costs a callee-saved register and a wider frame.

**Read a field back out after the `if`**, do not assign the local inside
it — fixes both the store's source register and gcc's jump threading of
a redundant re-test.

---

## Inline asm — correctness traps

**`j` to a label INSIDE an inline-asm block needs `.globl`.** gas
resolves it at assembly time and emits NO `R_MIPS_26` relocation, so the
linked jump is wrong. A correctness bug, not a mismatch.

**`move` in an asm block assembles to `addu`, not `daddu`** — spell
`daddu %0,$2,$0` out. gcc's own moves are fine.

**Even a ONE-INSTRUCTION asm block gets stolen for the `jr ra` delay
slot** — wrap it in `.set noreorder`.

**Retail clears and copies quadwords with `sq $0` / `lq $2`+`sq $2` asm
macros.** No C spelling reaches it: `v.q = 0` always emits `por`+`sq` at
a frame offset and loses the held address.

---

## Whole-file symptoms: check the TU boundary first

**A scheduling difference affecting a WHOLE FILE is a translation-unit
boundary, not a lever.** Run `python3 tools/tu_boundaries.py`. The
original left 415 `__gnu_compiled_c` markers and our tree has ~190 .c
files, so many of ours merge several original TUs — and per-file
compiler flags are the one thing no source lever can reach.

Confirmed twice, and **both splits wanted `-O2 -G0`** (i.e. WITHOUT the
SDK default `-fno-schedule-insns`):

  mpeg.c -> sceMpegDec.c              4 near-misses became matches with
                                      NO source change; one function
                                      21 differing words -> 0
  sceVif1Pk.c -> sceVif1PkRefLoadImage.c
                                      20 -> 14 diffs, and decisively
                                      19 wrong OPCODES -> 4

The fingerprint: **address materialisation interleaved AROUND an
intervening store is something `-fno-schedule-insns` can never emit.**

---

## Alignment and aliasing — a CORRECTNESS trap

**Use alignment variants of ONE struct tag, never two struct types.**
Two identically-shaped structs land in different gcc alias sets, and gcc
will hoist a read above the store that fills it. That is WRONG CODE, not
merely a mismatch, and byte-comparison against a function you have not
matched yet will not catch it. Declare `struct EVECS` once and typedef
it twice, one with `aligned(8)`.

**Member-level `aligned()` does nothing here.** ee-gcc takes block-move
alignment from `TYPE_ALIGN`, not `DECL_ALIGN`, so moving the attribute
onto declarations silently converts every `ld/sd` vector copy in the
file to `ldl/ldr`.

---

## Addressing and symbols

**An extern ARRAY's declared size decides gp-relative addressing.**
`extern short sym[];` (incomplete type) forces `lui`+`addiu` for its
address; giving it a small concrete size (`extern short sym[4];`) lets
gcc place it in small data and emit the single `addiu reg,gp,off` the
original has. Scalars get this for free, so it only bites on arrays.
Worth one word per reference site.

**Base-first vs offset-first addend.** `base[i]` reassociates to
offset+base (`addu v0,v0,s1`). Naming the address in a POINTER LOCAL --
`p = (int *)((int)base + i * 4); ... *p` -- gives base+offset
(`addu v0,s1,v0`). Each site needs its OWN local: sharing one local
across a call makes it live across the call and costs a dozen words.

**Spell a struct stride as its two halves when the original does.** A
0x450 slot made of an 0x400 array plus an 0x50 header appears in the
original as `(i << 10) + i * 80`, not as one 1104 multiply -- and it
does so even outside a loop, so this is a source spelling, not loop
strength reduction. `&tbl[i].member` folds to the single multiply and is
two words short.

**Prefer the base symbol whose displacement the original keeps.** Two
aliases for the same address (`slotTable[i].field` at base+0 vs
`workTable[i].field` at base-0x400 with a +0x400 displacement) generate
the same address but different code: the alias folds the offset into the
`%lo` and loses the shared `lui` the original hoists into a callee-saved
register. Pick the one whose load displacement matches.

## More control flow

**A REDUNDANT else arm changes basic-block layout.** `if (p) { X }`
inlines X between the test and the join. `if (!p) { q = 0; } else { X }`
-- where q was already 0, so the arm is a no-op -- makes gcc lay X out
AFTER the join and reach it with `bnez`+`b`, duplicating the join's
first load into both delay slots. That was a ten-word difference in
scEFFECT2Script.

**`goto` the default arm's assignment instead of repeating it.** A
switch arm that ends in the same value as `default:` gets its `move` sunk
into a branch delay slot; `goto` a label placed on the default arm makes
gcc branch to the shared block, which is what the original does.

**Raise register pressure EARLY to move address arithmetic off `$a0`.**
Reading a global into a local before a block of address arithmetic (even
with no call in between) can be the whole difference: in scMOVIEScript it
pushed the cursor pointer/value into `$t0`/`$t1` and left `$a0` free for
the call's buffer argument. Testing the global in place instead left
`$a0` free during the arithmetic and permuted eleven words.

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

## Process traps

**The `nonmatching` annotations in `asm/` are STALE.** splat wrote them
once and nothing updates them, so a function marked `nonmatching` there
may already be registered and byte-exact. **Check
`config/decompiled.txt`, never the asm, before starting anything.** An
agent began work on an already-matched function this way.

**Two symbols must never share a name.** One agent added a name that
already existed at a different address and caught it before committing —
a duplicate would have been a silent link hazard of exactly the kind
`tools/audit_dupes.py` exists to find.

**A stale "near-miss" comment on a function that actually MATCHES is a
real hazard.** One cost an agent 30 minutes and briefly broke a
registered match, because the comment invited experimentation on
working code. `checkfile.py` is the authority, not the file comments.

**A fixer-flag site index must be read off the POST-`fix_cc_asm`
stream, not the raw `ee-gcc -S` output.** Earlier passes insert nops and
expand macros, so an index taken from the raw assembly silently does not
fire. Use `tools/slot_sites.py`, which prints the word offset each
counting lands at.

**`set_flags.py` keys are BASENAMES, not paths.** Passing
`src/ssd/sef.c` used to write a key nothing ever reads, silently doing
nothing — three "the flag has no effect" cycles lost to it. The tool now
normalises and says so, but the fact matters when reading configure.py.

**A `static` duplicate is invisible to the linker but still wrong.** A
second `static` copy of a function does not collide at link time, so it
raises none of the usual duplicate symptoms — but callers in that TU
reach a local copy at a different address than retail. Found for
`DrawShadow` (static in tskMenuPause.c, global in Draw.c); replacing it
with an `extern` declaration kept the caller matching.

**GREP THE WHOLE TERRITORY for a function name before writing it**, not
just the file you expect it in. A function defined in two TUs has caused
two separate incidents: the linker shipped a mismatching `libc.c` copy
while verify.py checked the byte-exact one next door, and three `sef.c`
near-misses shadowed byte-exact `sefIs.c` versions so checkfile reported
them as registered-and-failing when they had never been broken. In both
cases the symptom pointed somewhere other than the cause.
`tools/audit_dupes.py` now runs in rebuild.sh.

**Never leave an entry registered-and-failing.** A registered entry
claims a byte-exact match; if it does not match, the ledger is lying and
the whole verification is worth less. Either match it or remove the
entry and document it as a near-miss.

**`tools/sbs.py` reads the BUILT object** and is stale unless
`checkfile.py` has run since your last edit. Run checkfile first — this
cost an agent a wrong conclusion.

**The source order that EQUALS the retail store order is not
necessarily the one that compiles to it** — in one case it was the worst
of all 120 permutations. Search, do not assume.

---

## From the task/screen state machines (wave 4)

**Hand the loop's initial value to LSR instead of writing the
accumulator.** When a loop-invariant constant is materialised in the
wrong place among the hoisted invariants, the fix is not another
statement order -- it is to stop writing the accumulator by hand.
`nX = -45; ... p[i].nX = nX; nX += 573;` makes gcc hoist the -45 AHEAD
of the loop's address bases; `p[i].nX = i * 573 - 45;` lets loop
strength reduction build the giv, and it emits the init LAST, which is
where the original has it. (`tskUmnDataBaseExWin`, 17 -> 13.)

**The `(unsigned short)` cast is what keeps a widened value alive across
a store.** Retail loading a halfword and then `andi 0xffff`-ing it is
not redundant work: the extension is a separate pseudo, and that pseudo
is what survives an intervening store to another object. Read the field
into an `int` local and compare through an explicit `(unsigned short)`
cast (or `& 0xffff`) and you get both the `andi` and the register that
crosses the store; drop the cast and gcc proves the value already fits,
deletes the extension, and then has to RELOAD the halfword after the
store. Its companion: if the original loads the same field twice with a
store in between, that reload is real -- the store may alias the global,
so the source really does read it again inside that arm.

**A field read that the original hoists ABOVE a run of stores has to be
written as a local.** gcc cannot move a load above a store to the same
object -- it must assume they alias -- so `w->win.nColor = w->nColor;`
written in place emits its load in the middle of the header block, while
the original loads the colour once as soon as the setup call returns and
holds it across every store. Reading it into a local right after the
call reproduces that, and because the local is then live across the
whole block, every other value in it lands one register later. That
knock-on is worth far more than the load: it was the last word between
`tskUmnMailMenu` and a match, and it fixed nine other differing words at
the same time. Symptom: one load in the wrong place plus a whole block
whose registers are uniformly shifted by one slot.

**Two fields of DIFFERENT modes assigned the same constant get two
registers.** `w->win2.nState = 1; w->bVisible2 = 1;` (a `sb` and a `sw`)
materialises the 1 twice; the original shares one register for both. A
single local assigned once and stored to both recovers it. This is the
mirror of the existing "two statements, not a chain" entry -- the
direction is decided by whether the original shares the register, and
both directions really occur. Worth checking the knock-on too: removing
that one word also removed a `.p2align` pad two switch arms later, which
is what made the function the right length.

**The same applies to a value that is stored AND passed.** When the
original keeps one register for a constant it both stores and passes as
an argument (`w->rib.nY = 48; eRibbonMain(&w->rib, 48);`), or for a field
it stores into another struct and also passes, naming it once is what
merges them; repeated literals and repeated field reads each get their
own materialisation for the argument. Six differing words in
`tskUmnDataBaseKeyWord` were exactly this. Naming is necessary but not
always sufficient: gcc will happily park the pseudo in a `$t` register
and copy it into the argument register at the call, one word per value.
When it does, a `PIN` to the argument register is the honest fix -- it
emits nothing, it degrades to a plain declaration in a portable build,
and it encodes a real fact about the original (that one register served
both roles). Watch the type while you are there: an `unsigned short`
local needs an `andi 0xffff` to promote for the argument, so if the
original has no `andi`, declare the local `int`. The same function can
want it both ways -- `tskUmnDataBaseExWin` needs the cast, its neighbour
`tskUmnDataBaseKeyWord` needs it gone. Read the original.

**Write the POSITIVE form when the original merges the two negative
arms.** `if (a == 0) x = 1; else if (b == 0) x = 1; else x = 0;` makes
gcc reach for a `bnezl` and lay the blocks out in the other order;
`if (a && b) x = 0; else x = 1;` gives two `beqz`es to a shared block
plus an explicit `b` over it, with the constant in each delay slot --
which is what the original does. Read it off the original: two
conditional branches to the SAME later address, and a `b` just before
that address, means the source tested the conjunction.

**A frame array that is quadword-copied wants a union, not a cast.**
`float mtx[16]` copied through `((TI *)mtx)[i]` is still a 4-byte-aligned
object to gcc, and its scheduler interleaves unrelated word stores into
the four `lq`/`sq` pairs. Declaring `union { float f[16]; TI q[4]; }`
gives the object the alignment it really has and the copy comes out as
one contiguous block. (`updateCursorMode1`, 17 -> 14.) A whole-struct
assignment is NOT the same thing -- gcc drops to word-sized pieces and
the copy gets seven words longer.

**"One local, two roles" also applies to loop-invariant constants.** In
`tskUmnDataBaseExWin` one `nX` served the row loop's invariant -196 and
the sprite loop's -45 accumulator. One C local is one pseudo, so the
shared register decided which constant was materialised first. Two
locals, with the one whose constant the original emits first assigned
first, fixed it.

---

## Loads, stores and aliasing

**A field read the original hoists ABOVE a run of stores has to be
written as a local.** gcc cannot move a load over a possibly-aliasing
store, so `w->win.nColor = w->nColor;` emits its load mid-block. Reading
it into a local right after the setup call was the last word of one
function — and shifted every other register in the block by one slot,
fixing nine more words at once.

**A second load of the same global AFTER a store is REAL** — the store
may alias, so the source really does re-read it in that arm.

**The `(unsigned short)` cast is what keeps a widened halfword alive
across a store.** Without it gcc proves the value fits, deletes the
`andi`, and has to reload after the store.

**Two fields of DIFFERENT MODES assigned the same constant get two
registers** (`sb` + `sw`); one shared local recovers retail's single
register — and dropping that word can also drop a `.p2align` pad two
switch arms later, fixing the length.

**Hand the loop's initial value to LSR.** `p[i].nX = i * 573 - 45;` lets
strength reduction build the giv and emit its init LAST, where retail
has it; `nX = -45; ... nX += 573;` hoists the -45 ahead of the loop's
address bases.

**A quadword-copied frame array wants a UNION, not a cast** —
`union { float f[16]; TI q[4]; }` gives it the alignment it really has
and stops the scheduler interleaving unrelated stores into the `lq`/`sq`
block. A whole-struct assignment is NOT the same (gcc drops to word
pieces, +7 words).

**Write the positive `if (a && b) x = 0; else x = 1;`** when the
original merges two negative arms; the negated form makes gcc reach for
`bnezl` and flip the block order.

---

## Narrow values and small loops

**Nested `if (n >= 0) { if (n < 3) ... }` beats `&&`.** On an `lbu`
value the `&&` form folds to one `bnez` — gcc drops the sign test when
the comparisons are adjacent. Worth ~30 diffs each on two functions.

**A tiny counted search loop needs its first iteration peeled by hand.**
`i = 0; do { ... i++; } while (i <= 0);` has a back edge gcc 2.96 proves
dead and deletes (~14 words). The original build kept both the peel and
the unreachable second iteration.

**A bare `extern char` scalar goes through `$gp` at -G8.** Declaring it
`extern char X[0x10]` restores the original's absolute `lui`/`sb`.

---

## Fixer site indices are NOT diff indices

**This has caused at least one function to sit parked with the answer
one index away.** The three families count different streams:

| flag | counts |
|---|---|
| `--swap-adjacent`, `--rotate`, `--rotate-seq` | the POST-`fix_cc_asm` stream, before gas steals a delay slot |
| `--swap-regs` | gcc's OWN assembly output |
| `--short-loop-pad` | all-nop `.set noreorder` blocks in the post-fixer stream |

None of them counts the word offsets you read off a `scratch_diff.py`
diff. `sceMcRename` was parked because sites 54-58 were swept off the
diff when the answer was 53. `python3 tools/slot_sites.py` prints all
three countings with the word offset each lands at — but note it counts
BRANCHES, so it does not list short-loop pads; walk the emitted stream
for those.

**Worth re-checking parked near-misses in other territories against the
right numbering before concluding a flag "does nothing".**

---

## The pre-RA scheduler decides register numbering

`-fschedule-insns` runs BEFORE register allocation. When two hard-register
argument copies bracket a compare, whichever copy the pre-RA scheduler
hoists takes its register first, and `local_alloc` then hands the
combine-folded compare temp **the lowest still-free number**. If the
original hoists the other copy, no source shape can reach it — proven by
blocking the taken register with a PIN'd dummy, which moved the temp to
the NEXT free number rather than the wanted one.

That is a `--swap-regs` case, and often a RANGE-scoped one: the same
registers elsewhere in the function are ordinary ABI arguments that must
not move. `--swap-regs FUNC:A-B:LO-HI` exists for exactly this.

**"Registers AND order wrong in the same two words" = rename then
FORCE-swap.** Neither fixer alone reaches it.

---

## Store runs and rotations

**A run of byte stores emits as a LEFT-ROTATION of source order — and
the rotation DISTANCE is per-block.** Measured at two in one block and
three in another. Measure it rather than assuming; then write the source
rotated the other way.

**Colour bytes patched into a string want the stores DUPLICATED in both
arms** of the `if`. Selecting into one local yields `movz`; duplicated,
gcc cross-jumps the arms back into retail's single store run.

**A local pointer array with an INITIALISER LIST is what produces
retail's unaligned `ldl/ldr` block copy**; a static template plus an
explicit `memcpy` gives aligned `ld/sd` instead.

**Where a shared tail store goes — in each arm, or after the merge —
decides the callee-saved assignment for the WHOLE function.** Worth 47
words in one case.

**A held pointer to an already-addressable member can COST words** —
seven, measured across all 16 use combinations in one function. It is
not a free lever; measure both ways.

---

## Narrow types at stores and compares

**A `short` local whose only fate is an `sh` costs a `sll`/`sra`
sign-extend pair** — declare it `int`.

**A `(short)` cast on an `unsigned short` field selects `lh` at a
comparison** while other reads of the same field stay `lhu`. Retail
mixes both on one field, so read each use separately.

**`LAUNDER` inside an if-body blocks gcc's if-conversion to `movn`** and
restores the original's real branch. Nothing else reached it.

**An unsized `extern` array indexed `[0]` keeps a store off `$gp`** where
a plain `extern int` costs the `lui` — the mirror of the `extern char`
entry above.

---

## Build hazards

**A block-scope `extern` is a latent build bomb.** gcc 2.9x keeps such a
declaration's name on the function obstack, frees it, then prints the
dangling pointer in its `-G8` `.extern` directive. It stays harmless
only while the freed memory happens to hold a live symbol name — adding
one unrelated function turned it into `.extern ;, 4` and gas rejected
the whole file. Hoist every `extern` to file scope; it is
codegen-neutral.

---

## Volatile, varargs and sibling calls

**A record reloaded before EVERY field store is `volatile`.** No
aliasing story explains retail reloading both a count and a base before
each store; the declaration does. That alone matched six functions.

**A `volatile` store will NOT fold a byte offset into its store offset —
but a volatile struct MEMBER will.** Indexing does not help; only the
member does.

**`PASSTHRU` breaks the CSE that decides which of two equal values a
test reads.**

**`va_list` must be `__builtin_va_list`** — a `char *` typedef spills.

**`n >= 0 && n <= K` folds to one unsigned compare** — retail's `bltz`
needs two nested ifs.

**A void call becomes retail's `j` sibling jump only as the function's
LAST statement, outside any `else`.**

---

## Axes that DON'T work — don't spend runs on these

**Local declaration order does not move gcc 2.96's allocation.** Four
orders, byte-identical output. Measured, not assumed.

**The five window-header stores ending every Menu list-change function
are scheduler-ordered** — all 120 permutations give an identical diff.

**`-fno-gcse` has no effect** on the "original recomputes still-live
subexpressions" class: that is `cse_main`, not gcse. Verified per-file.

**`-fno-optimize-sibling-calls` on Enemy.c** restores one `jal` but
costs eight other functions in the file (27/2 -> 20/9).

---

## When a lever is not the answer## When a lever is not the answer

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

**`--unfill-gcc-slots` only sees `.set noreorder/nomacro` blocks.** When
gcc emits a branch in REORDER mode, *gas* fills that slot, not gcc, so
the flag cannot touch it — and applied whole-function it will unfill the
`bgez`/`bltz`/`jal` slots the original genuinely does fill. `--pin-slot-nop
FUNC:N` is the right flag there. A related freebie: once the slot holds
a nop, gcc's own `.p2align 3,,7` emits any needed trailing pad without a
flag at all.



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

**A short run of independent stores does not come out in source order.**
Three field stores closing a UI panel had to be written
`nWidth / nMode / bFlags` to be emitted in the original's
`nWidth / bFlags / nMode` order -- writing them in emitted order gives
the full reversal instead. Do not reason about it: permute exhaustively.
Three statements is a six-case script, four is twenty-four, and this has
now paid off in two separate files.

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

## Sound driver, run 4 (sef/sdv/srs)

**Take the element address into a local inside the loop body to defeat
strength reduction.** `p = &tbl[i]; if (p->a == x) p->b++;` keeps the
original's per-iteration `base + i*stride` recomputation and its reload
of the loop bound. Spelled with plain `tbl[i].a` subscripts, gcc builds
a walking pointer and hoists the count -- 5 words shorter.
(sefSetHitSignal, sefSetSeSignal.)

**...and the reverse.** Two plain subscript accesses to the SAME element
in one loop (`if (buf[i] == 0) { buf[i] = 1; ... }`) make gcc build TWO
givs off a copy of the base, which is what the original has; one walking
pointer is 2 words shorter. (sdvAllocSpecialWork.) So the choice between
subscript and walking pointer is a real two-way lever -- read the
original's address arithmetic before picking.

**Name the table base.** `t = &_battleActor;` then `t->aActor` /
`t->nActors` makes the base the master register and the walker a copy;
using `_battleActor.aActor` directly gives the mirror-image assignment.
(sefSetLightFlag.)

**LAUNDER_V on a value reloaded at the end of a loop body** fences the
EE scheduler, which otherwise issues the loop-counter increment ahead of
the reload no matter what the source order is. (sefSetLightFlag.) It
does NOT work when the same value is also loaded in the loop preheader:
there gcc keeps it in a register and adds `move` copies (swept in
sefCaclAllTarget).

**LAUNDER a pointer that was just tested against null** before passing
it as an argument, or gcc folds it to `$zero` and costs a `move a1,zero`
the original does not have. (sefInitEffectBattle.)

**A stack record wants to be a struct local, not a `char[]` behind a
pointer variable.** The pointer costs an extra callee-saved register and
six words. Pad the struct out to its real size and `memset(&r, 0,
sizeof(r))`. (sefDeleteEffectWait.)

**`(float)(int)(x & 0x7FFF)`, never `(float)(unsigned)`.** There is no
unsigned-to-float instruction, so gcc emits a branchy halve/or/add.s
sequence: sefGetCubePosBtm went 62 -> 96 words. The intermediate cast is
load-bearing.

**An inlined LCG wants one local per draw.** `nA = seed*M+C; nB = nA*M+C;
nC = nB*M+C;` reproduces the original's v0/v1/a1 chain; reusing a single
`seed` variable funnels every intermediate through v0. The seed must be
`unsigned` or the extract is `sra`, not `srl`. (sefGetSpherePos.)

**A single-use float local can fix a schedule.** In sefGetOfsRange
`(float)pPrm[0]` written inline in a call argument is scheduled after
the seed update (10 words out); hoisted into a local that another arm of
the function also uses, it spills to a second callee-saved float (27
words out). A FRESH single-use local gets both the placement and the
register right.

### VU0 in the sound driver

The samplers' trig and vector math are VU0, not compiler output, and the
same shapes repeat -- see the VU_SIN/VU_COS/VU_ITOF/VU_MUL/VU_SUB/
VU_SCALE macros at the foot of src/ssd/sef.c.

**Every one of those asm bodies MUST carry `.set noreorder`.** Without
it gas inserts its own hazard nop (e.g. between `mfc1` and `qmtc2`) and
the function is long by exactly that many words. Leave the trailing
`mtc1 -> mul.s` pad to gas, though: that one matches.

**Pass a stack quadword with an "m" constraint** to keep the original's
`lqc2 $vf1, 0(sp)`; an "r"(&x) operand can force an address into a GPR.
(sefGetSpherePos.)

### Registered-and-failing hazard

sefSetLightFlag / sefSetHitSignal / sefSetSeSignal existed TWICE tonight:
non-matching near-misses in sef.c and matching versions in sefIs.c. Two
definitions of one symbol in two translation units -- the build still
linked, but `checkfile.py src/ssd/sef.c` diffed the stale copies against
the registered entries and reported registered-and-failing. Before
writing a function, grep the whole territory for its name, not just the
file you expect it in.


---

## From the nml (model / packet) run

**A `volatile` cast costs the gp-relative store.** `*(volatile int *)&g =
0;` makes gcc materialise the address; plain `g = 0;` uses the
small-data `sw $0,-N($gp)` the original emitted. `nmlPacketGsInit` was
registered-and-failing for exactly this, and the fix was DELETING the
steering.

**`--unfill-gcc-slots` only sees noreorder/nomacro fill blocks.** gcc
emits a plain `b` in REORDER mode and lets gas fill the slot; the flag
never touches those, and applied whole-function it unfills the
bgez/bltz/jal slots the original DOES fill. For a gas-filled slot the
tool is `--pin-slot-nop FUNC:N`, whose N counts only reorder-mode
branches.

**A `.p2align 3,,7` pad nop appears by itself once a slot holds a nop.**
gcc aligns after a loop/branch; the extra trailing nop that looks like a
second thing to reproduce is just the alignment shifting. Fix the delay
slot and stop counting nops. (608 bytes in one edit:
`nmlModelSetActiveFadeOut` / `...FadeIn`.)

**Get the FRAME SIZE right before anything else.** A stack buffer handed
to an SDK filler pins its own declared size: `sceGsSetDefLoadImage`
uploads five quadwords but the original reserved SIX, and until the
frame was 144 rather than 128 every other diff was noise.

**Read a table INDEXED, not walked, when the original has an `addiu` +
`move` pair before the loop.** `pOfs[i]` makes loop strength reduction
build a giv whose initial value is a separate pseudo (`addiu $16,$18,48`
then `move $17,$16`); `*pOfs++` folds both into one `addiu` and comes out
two words short. This is the mirror image of the "walked tables want a
block-local pointer" lever above -- check which side of it you are on.

**`addu rd,<int>,<ptr>` needs integer arithmetic in the source.** Every
spelling of pointer-plus-int canonicalises to `addu rd,<ptr>,<int>`;
`(T *)(n + (u_int)p)` is what produces the other operand order.

**A short-truncated sum of an int field wants its own int local.** Passed
as an expression to a `short` parameter, gcc's combine folds the
truncation back into the load and emits `lhu` where the original has
`lw`. Assigning the sum to an `int` local first blocks it -- worth 4
words in `nmlPacketTextureTrans`, and it is a natural source shape, not
steering.

**Arm order tells you the source condition.** gcc emits the THEN block
inline and branches to the ELSE. So a branch-LIKELY (`bnezl`) jumping
past a call means that call is the THEN arm and the condition in the
source is the one you would have written second. Swapping the two arms
of `nmlPacketAddTexture`'s inner dispatch took it from 42 diffs to a
match in one edit.

**A rotated loop's reset lives in the bottom branch's delay slot.**
`move $t0,$0` in the delay slot of the loop-back `bnez $t0` is the loop
body's FIRST statement, duplicated into the preheader -- i.e. the source
is `flag = 1; while (flag) { flag = 0; ... }`. Writing it as
`do { flag = 0; ... } while (flag)` instead costs a word.

---

## From the TMENU run

**The declared RETURN TYPE of a void-result call is a register-allocation
lever.** `TMENU_init` ends with nine `EW_addComponent(...)` calls whose
results are unused. Declared `void`, gcc put an unrelated
read-modify-write value in `$v0`; declared `int`, it moved to `$v1` and
the function matched outright. Nothing else changed -- same call, same
arguments, no extra instruction. Nine diffs to zero. When a near-miss is
pure `$v0`/`$v1` naming around a call, check the prototype's return type
before reaching for a pin.

**Advance a pointer local immediately after storing it to kill the CSE
that suppresses a reload.** Retail's `TMENU_init` stores `t+0x198` into
`t->ppLine144` and then *reloads* `t->ppLine144` for the block copy that
follows. gcc will not reload while the stored value is still live in a
register. Writing `t->ppLine144 = pLine; pLine += 3;` -- the same local
advanced to its next role right after the store -- kills the copy and the
reload comes back. 69 diffs to 39 in one edit. (The `pLine += 3` is also
the real source of `t->pText154`, so this is not steering.)

**A constant that must cross a call has to be a local defined before it.**
`t->nItemMax = 16;` written after `MBUF_create` materialises the 16 in a
caller-saved register after the call; retail has `li s0,16` in the
prologue. `int nMax = 16;` declared with the other locals and stored
after the call reproduces it -- gcc 2.96 does *not* constant-propagate a
local whose live range spans a call. 78 diffs to 69.

**A byte field read for both the test and the arithmetic wants the local
INSIDE the arm.** `n = t->b141; if (n) off = n*24+12;` loads straight
into the multiplicand and drops retail's `move v1,v0`. Written
`if (t->b141) { n = t->b141; off = n*24+12; }` the field is CSE'd into
one pseudo, the arm copies it, and the copy plus its `.p2align` pad come
back -- worth two words of LENGTH, not just registers.

**`li at,0xffff` + `addu` means the source really adds 65535.** A
16-bit-wrapped state test spelled `(unsigned short)(v - 1) < 2` compiles
to a single `addiu -1`; retail's two-instruction form only comes from
writing the addend as `0xFFFF`. Symptom: one word short and a
LENGTH triage on an otherwise perfect prologue.

**Dual-width access to one struct member is a UNION, never two structs.**
An EW component's word at +0x10 is written `sw` from one caller and `sh`
from another, and +0x14 likewise with a `short` at +0x16 inside it.
`union { int n; short h; struct { short h0, h2; } w; }` keeps every view
in one alias set; two struct types would let gcc hoist a read above the
store that fills it.

**Check the struct's own ALIGNMENT when a `char pad[]` stops working.**
Giving `MSGQUEUE` a leading `int *` raised its alignment to 4, so its
size rounded 0x12 up to 0x14 and every member after it in the enclosing
struct moved by four bytes -- diagnosed as IMMEDIATE diffs in three
*already matching* functions. The pad after an embedded struct has to be
computed from the ROUNDED end, not the last member's offset.

---

## Translation-unit boundaries (SDK)

**A whole file that is one word off everywhere is the wrong compiler
flag, not a lever problem.** The MPEG2 decoder helpers around 0x2107B0
would not stop interleaving their address materialisation around an
intervening store -- something `-fno-schedule-insns` can never emit.
Splitting them out of `mpeg.c` into their own TU (`sceMpegDec.c`, at
`-O2 -G0`) turned four near-misses into matches with NO source change,
and the parked ones dropped from 21 to 4 differing words. Symptom: the
first scheduling pass's fingerprint -- an unrelated instruction sitting
between a `lui` and its `ori`, or between a `%hi` and the load that uses
it. `__gnu_compiled_c` symbols in the ELF mark the boundaries; a
function whose neighbours in address order are in a different file is a
candidate for its own TU.

**Constants used only AFTER a loop, but living in callee-saved
registers, mean the code that uses them is INSIDE the loop.**
`_nextHeader` hoists the callback message's `5` and `-1` into `$s6`/`$s1`
ahead of a scan loop; that only happens if loop-invariant motion saw
them, i.e. the picture-header tail is a `return` from inside the loop
rather than code after a `break`.

**A four-way dispatch with an `sltiu` midpoint test is a `switch`, and
the case blocks come out in SOURCE order.** `_nextHeader` went from 29
differing words to 2 purely by moving one `case` above another; the
decision tree is fixed by the label values, but the out-of-line block
layout follows the order the cases are written.

## Statement-order levers

**Reusing ONE variable across both arms makes gcc reuse its register.**
`_outputFrame` kept `picture_structure` and `progressive_frame` in two
locals and gcc gave the second a fresh `$a0`; assigning both to the same
`int nType` (reloaded at the head of each arm) let the second load reuse
`$v0` -- last three differing words, gone.

**A run of `sw zero` stores comes out ROTATED RIGHT by one from source
order.** Twice (`_skipMB0`, `_sliceA0`) a block of zero stores matched
only after rotating the source LEFT by one position. Write the desired
output order, move its first store to the end, and the delay-slot filler
puts it back.

**`LAUNDER` on an intermediate defeats cross-jumping without moving the
result's register.** Two arms ending in an identical `slt; xori` get
folded into one block (one word short). Laundering the *operand* the arm
computes (`nArea = w*h; LAUNDER(nArea);`) splits them; laundering the
*result* splits them too but hands the boolean a callee-saved register
one slot earlier and costs five register diffs instead.

**A constant address whose low half is zero folds its displacement into
the load -- but only through a pointer VARIABLE.**
`*(volatile long long *)0x10002030` gives `lui/ori/ld 0(r)`;
`pRegs = (volatile long long *)0x10000000; pRegs[1030]` gives the
original's `lui/ld 0x2030(r)`, because gcc constant-folds the address
back together if you write it as one constant. Worth 26 differing words
in `_ipuVdec`.

**gcc 2.9x sibling-calls a void tail call.** `void f(p){ g(p, 3); }`
compiles to `j g` with no frame at all; if the original has
`addiu sp,-16 / sd ra / jal g / ld ra / jr ra`, no argument shuffling
will reproduce it (`_dmVector`, parked at 7 words).

---

## Recovering the SHAPE from the object, not the diff

Three levers from the tsk/UMN screen-task run. All three are about
reading facts out of the original binary instead of guessing a source
shape and iterating on the diff.

**READ THE JUMP TABLE to find where one `case` ends and the next
begins.** A `switch` on a state byte compiles to a table of absolute
addresses; each entry is the exact first instruction of that arm. If
entry[N+1] is three instructions past entry[N], arm N is three
instructions long and FALLS THROUGH -- and if a statement sits between
them in the object, that statement belongs to arm N even when it reads
like the top of arm N+1. In `tskUmnMailHensin` the first
`eCursolModeChange` call looks like the head of state 10 and is
actually the tail of state 0; with it on the wrong side gcc hoists the
argument address into a callee-saved register and the function is one
word long. Dump the table with the ELF, not with objdump on the text.

**A `b` whose target is one instruction PAST the next block's first
instruction means the previous arm falls through into it.** The
fallthrough path already has some address or constant live in a
register, so gcc skips the `addiu` that the jump-table edge needs and
enters the arm one instruction late. Reading that as "the arm ends with
`break`" costs a word and, worse, hides that the shared pointer wants a
callee-saved register. Closed `tskUmnPluginList`, and it is what the
`b <state21+4>` in `tskUmnDataBaseMenu` means too.

**A two-way dispatch on a byte is a `switch`, not an `if`/`else if`.**
gcc emits a switch's compare chain first and both case bodies after it;
an `if`/`else if` inlines the first body between the two compares. Same
semantics, four words apart (`tskUmnMailMenu`).

## Permute the statement GROUPS, do not reason about the scheduler

gcc 2.9x's basic-block scheduler does not emit a run of independent
stores in source order, and the order it *does* pick is not predictable
by hand -- but it is a function of the source order, so it is cheap to
search. Split the run into the 3-5 groups that share a materialised
constant or a base register and try every permutation with
`checkfile.py`; each run is ~20 s, so 24 orders is ten minutes and 120
is under an hour. Measured on this run:

- `tskUmnPluginList`'s init block: 26 diffs -> 12 over 24 orders (only
  one order is right, and it is not the one that reads naturally).
- the three row-table stores after its build loop: 12 -> 6.
- `tskUmnMailHensin`'s three cursor stores: 28 -> 17.
- `tskUmnMailMenu`'s two window arms: 163 -> 30 over 16 orders.

Two rules of thumb from those sweeps: the win is usually a *rotation*
of the groups rather than a swap of two adjacent statements, and a group
that stores a value LOADED from another field (`w->win.nColor =
w->nColor`) wants to move as a unit with the stores that share its
register. Script it (see `scratchpad/tsk2/perm*.py`) rather than editing
by hand -- and use an anchor that includes the enclosing function's
signature, because short declaration runs like
`short nTargetOut;\n    int i;` repeat across functions in one file.

## Conversions at a prototyped call

**`lbu` at a call site means the argument is NOT converted to a narrow
signed type.** Passing an `unsigned char` to a `signed char` parameter
makes gcc emit `lb` (it folds the sign extension into the load); the
original's plain `lbu` means the parameter is `int` and any narrowing is
an explicit cast at the OTHER call site. `UmnPluginTextGet` in tskUmn.c
is the case: `int` parameter, `UmnPluginTextGet(plugin_folder[i])` in
`tskUmnPluginList` (`lbu`), and
`UmnPluginTextGet((signed char)UmnWork.u.plugin.nSel)` in
`tskUmnPluginInfo` (`lbu` for the compare, then `sll`/`sra`). Changing
the prototype to `unsigned char` or `signed char` breaks one or the
other; only `int` plus the cast satisfies both.

**A `j` to a label defined inside the same inline-asm block needs
`.globl`.** gas resolves the jump at assembly time and emits NO
R_MIPS_26 relocation, so the encoded target is the section offset and
the linked jump lands in the wrong place -- a CORRECTNESS bug, not just
a one-word mismatch. Declaring the label `.globl` inside the block
forces the relocation; checkfile then masks the field and the function
matches. (`_ModelCalcClip` and friends.)

**`move` inside an inline-asm block assembles to `addu`, not `daddu`.**
Retail's register copies are `daddu rd,rs,$0` (0x2d); the `move` macro
in an asm block gives 0x25. Spell `daddu %0, $2, $0` out. gcc's OWN
moves are unaffected -- this only bites hand-written asm text.

**Five pointer arguments arrive in `$a0-$a3` plus `$t0`** in this
image's ABI, so a whole-body-asm leaf with a fifth operand needs no
stack load (`_FacePoint`).

**Which of two `x | c` values reuses the shared temp's register is decided
by which variable you SEED with the temp.** When two constants are OR'd
onto the same value, gcc computes the zero-extend/copy once and folds one
of the two ORs in place over it; the other gets a fresh register. Writing
both as independent expressions off `n` lets gcc pick (it picks whichever
target is already a hard register, e.g. a `PIN`). To choose, write
`gt = (unsigned long)n; eop = gt | C1; gt |= C2;` -- the `|=` variable is
the in-place one. (sceVif1PkRefLoadImage.)

**An empty asm is the only way to keep a combine-folded compare temp
alive -- and it buys the register at the price of the schedule.**
`PIN(unsigned int ne,"$5"); ne = n ^ qwc; if (ne == 0) ...` is silently
IGNORED: combine merges the xor into the `movz`, the pseudo dies, and the
register variable evaporates. Adding LAUNDER / LAUNDER_V / PASSTHRU /
PASSTHRU_V does make the PIN stick (triage flips REGISTER -> SCHEDULING,
i.e. the multiset becomes exact) but every asm form is a hard scheduling
barrier for ee-gcc 2.9, costing 9-14 words of order. Only worth it when
nothing downstream needs to move.

**A whole-file scheduling fingerprint means a TU boundary, and
`tools/tu_boundaries.py` will tell you.** Sony's SDK archives are one
object file per function, so a single function can legitimately have been
built with flags none of its siblings used -- which one .c file cannot
express. Split it into its own .c (auto-discovered by configure.py) and
sweep flags with `tools/set_flags.py --cflags`. Confirmed twice: mpeg.c ->
sceMpegDec.c (four matches, no source change) and sceVif1Pk.c ->
sceVif1PkRefLoadImage.c (20 -> 14 diffs on the flag alone, then 4 once the
register levers could land). Both wanted `-O2 -G0`, i.e. the SDK default
`-fno-schedule-insns` removed.

**Scope a pointer local to the switch arm that uses it.** A single
function-wide `T *p` shared by several arms of a state machine keeps the
allocator from ever giving one arm the original's register -- it sees one
long live range. Redeclaring it inside the arm's own block recovered
retail's `$a0` and closed 3 words of `TMENU_updateDefault`. It is a
PER-ARM lever: the same rewrite in a second arm of the same function
changed nothing, so try it arm by arm rather than globally.

**Write a `switch` by OMITTING the dead cases, not by spelling them.**
gcc emits one jump table spanning min..max and fills every hole with the
default label, so six `case` labels reproduce retail's 16-entry table
exactly. Both of `TMENU_updateDefault`'s h14 dispatches and its
queue-control dispatch came out right first try this way.

**`t->shortfield = t->shortfield + 1` keeps the value RAW; a `short`
local does not.** Member-to-member gives `lhu`/`addiu`/`sh` and defers
the sign extension to each later use (`sll`/`sra` AFTER the add), which
is the usual retail shape; routing it through a `short` local makes gcc
extend at the definition (`sll`/`sra` BEFORE the add). Read the original:
whichever side of the `addiu` the extension sits on tells you which
spelling the source used. Same rule for `signed char` members in a
read-modify-write chain -- use the member, not a local, and CSE forwards
the stored byte raw exactly like retail.

**A store through a differently-typed pointer kills the CSE of a struct
member across it.** `p->field = x;` (EWCOMP*) between `t->h30 = ...` and
a later `t->h30` read forces a genuine reload. Put the member's re-read
BEFORE the foreign store, or move the foreign store above the member
store -- both orders were needed in different arms.

**gcc deletes the first of two stores to the same member even with a
non-aliasing load between them; only a `volatile` store keeps both.**
Retail's own barrier was a dead load of a neighbouring member, which our
build's alias analysis sees straight through. Spelling the first store
`*(volatile int *)&t->nFlags = v;` restores both stores (and was the
last word of length in `TMENU_updateDefault`) at the price of a register
naming shift.

**Two `return` arms with identical tails get cross-jumped -- retail was
saved by register allocation, you are saved by permuting one arm's store
order.** The merge silently costs several words and looks like a "length
short" bug. Permuting the stores in ONE of the two arms broke it and was
worth 2 words plus the length match.

**`--mtc1-nop FUNC:N` is needed twice per `(float)n * k * m` block.**
ee-as pads two slots after the LAST `mtc1` of a float-constant pair
before the first COP1 compute; this toolchain emits only the first
hazard nop (after the int->float `mtc1`). Count every `mtc1` of the
function in emission order and name the last one of each block.

**Local DECLARATION ORDER does not move gcc 2.96's register allocation.**
Four orders of `TMENU_updateDefault`'s twelve locals gave byte-identical
output. Do not spend compile runs on this axis.

## From the xgl run (Cd / Font / Movie / Hdd)

**A statement AFTER a call in tail position blocks gcc's sibling-call
jump.** gcc 2.96 turns a trailing `f(a, b);` into `j f` (restoring `$ra`
and `$sp` first); the original build emits `jal f` plus a branch to the
epilogue. ANY statement after the call suppresses it -- a dead
`nSize = 0;` or `nWord = 0;` costs zero instructions because the store is
dead. This is NOT a whole-file flag question: `-fno-optimize-sibling-calls`
on xglCd.c fixed `extract` but broke three functions whose sibling calls
the original DOES have, so the original compiler had the optimisation and
the difference is genuinely in the source. Check the callee's ARITY too --
`extract` only matched once xglArxExtract was declared with two arguments
instead of three; the spurious third made gcc copy the caller's third
argument out of `$a2` and rotated every register in the function.

**`if (cond) { body; return 0; } return 1;` and
`if (!cond) return 1; body; return 0;` emit the same instructions in a
DIFFERENT ORDER.** With the body inside the `if`, the return constant is
the FIRST of the invariants gcc hoists above the branch; with the early
return it is the LAST. Worth 3 words in `queue_next`, and nothing else
moved. Try both before reaching for a scheduling flag.

**A guard written as `||` puts its LAST arm's block on the fall-through
and lets gcc fold the first adjacent pair into one `sltiu`.**
`if (nX < 0 || nX >= 768 || nY < 0 || nY >= 512) { A } else { B }` is a
different program layout from `if (in range) { B } else { A }` even
though the arms are the same: the OR form makes A the fall-through and
gives `sltiu $v0,nX,768` for the first pair. Read which arm the original
jumps FORWARD to and write that one as the branch target. (set_xyz;
xglCdDiskCheck is the same lever at whole-function scale -- writing the
rare arm as the `else` recovered the shared tail, the callee-saved copy
of the constant 1 and a value held across a call, 80 diffs to zero.)

**KNOWN BLOCKER -- the R5900 SHORT-LOOP PAD is 8 in this toolchain and
10 in the original build.** ee-gcc 2.96 pads a tight loop containing a
branch-likely out to 8 instructions from branch target to closing
branch, inclusive, by emitting `nop`s between the loop-closing load and
the branch. Several original loops are padded to 9 or 10 instead.
Verified it is a fixed TOTAL, not a fixed nop count: adding real
instructions to the body drops the pad to zero at 8 total. No source
shape moves it (while / do-while / for / goto, char vs u_char, named
temp vs direct compare all give the same pad), `-m5900` and
`-falign-loops=` do not change it, and `SCHED_NOP()` lands BEFORE the
closing load rather than between the load and the branch. There is no
fix_cc_asm pass for it. Two otherwise-perfect functions are parked on
exactly this (`FileSelectListReload` 70/72, and it also blocks
`xglCdStreamOpen`); an opt-in `--short-loop-pad FUNC:N:COUNT` pass is
the way in.

**`--swap-regs` is WHOLE-FUNCTION.** When a near-miss is "6 of this
function's 14 $v0/$v1 uses are swapped", the flag makes it worse, not
better. In `fileRead` that exact residue fell instead to declaring the
preceding call `void` rather than `int` (the return-type lever) -- check
that first, always.

---

## Added by the game/SEQ sweep

**Write a shared trailing store into BOTH arms of an if/else, not once
after it.** With a single-store `else` arm gcc sinks it into an
annulling `beqzl` delay slot and you come out 2 words SHORT per site;
duplicated in the source, gcc cross-jumps it and fills the delay with
the shared computation instead.

**A lone `if (i == N) i = N-1;` comes out as a `movz`** — give the arm
its own store and gcc branches, plus tail-duplicates the way the
original does.

**A boolean field store must be if/else with a store in EACH arm**,
polarity chosen so the branch is `bnez` to the `= 1` arm. `?:` gives a
branchless `sltu`, three words short.

**Write a shared trailing store into BOTH arms, not once after the
if/else.** When the `else` arm holds exactly one store and the tail
holds another, gcc sinks the else-arm store into an annulling `beqzl`
delay slot and the function comes out two words short per site.
Duplicating the tail statement into each arm makes gcc cross-jump the
tail instead and fill the delay slot with the shared computation --
retail's `beqz` / `ori` / `nop` / `sw` shape. Worth 6 words in
`SEQ_scale`, and it is the same trick that closed `RSRC_dispose`
(`pItem->nState = 0;` inside both arms).

**Hoist a reciprocal that has to survive a call into its own
statement.** `nearDir(a, b) * (1.0f / (float)n)` lets gcc sink the
int-to-float conversion past the call, which forces `n` into a
callee-saved integer register, spends an extra `$s` register and shifts
every other assignment (132 diffs at the right length in `SEQ_rotate`).
`float k = 1.0f / (float)n;` written before the call puts the
reciprocal in `$f20` across it and matches.

**Multiply through named scalars, not through the vector elements you
just stored.** In the bone-constraint transforms retail loads the three
components in 16/20/24 order; writing `v[0]*m[0] + ...` lets the
scheduler hoist the 24 load first. `x = c->fOffset[0]; ... o[0] = x*m[0]
+ ...` pins the order. (SEQ_transCNSUnitChr.)

**A member reached through a POINTER LOCAL and the same member reached
through the parent are not CSEd.** `pPos[1]` (where `pPos = u->fPos`)
and `u->fPos[1]` stay two separate address computations in gcc 2.96.
This is normally a hazard; in `SEQ_rotateUnit` it is the only way to
reproduce retail's degenerate self-aiming `xglAtan2` calls, which read
the unit's own position through the member while the subtrahend comes
through the pointer local.

**Read a nested member as the FULL chain when retail folds the offsets.**
`p->mov.cns.nBone` gives `lw v1,120(p)` (56 + 64 folded, scheduled
early); the same field through a channel pointer gives `lw a1,64(c)` and
sinks past the loads that depend on it. 30 diffs -> 3 in
`SEQ_transCNSUnitChr`.

**A block-scoped held-address pointer, but ONLY block-scoped.**
`ANM *w = &a->anim; w->nFlags |= 8;` recovers retail's `addiu s3,s0,1776`
+ `0(s3)` form where `a->anim.nFlags` gives the `1776(s0)` offset form
and comes out short. Declared at function scope instead of inside the
guarded block, the same pointer becomes a multi-block pseudo and the
function goes from matching to 44 diffs. (SEQ_moveSPL.)

**Drop your own zero-trip guard around a loop.** An explicit
`if (n != 0) { for (...) }` duplicates the guard gcc emits anyway, as a
second `beqzl` with a stolen delay slot. (RSRC_dispose.)

**A SEPARATE loop variable seeded from a clamped index.** `nIndex` for
the clamping and `for (i = nIndex; ...)` for the walk costs exactly the
two `move` words retail spends; one variable for both comes out two
words short. (RSRC_info.)

**Every non-zero return must funnel through ONE variable.** Separate
`return x;` statements where the compiler can prove `x == 0` get folded
to `move v0,zero` individually, and retail's callee-saved result
register disappears. Restructuring `RES_loadFile` to a single result
local took it 90 diffs -> 16.

**Reminder on the %hi/%lo pair.** `lui a1,0x37` + `addiu a1,a1,-19896`
is address 0x36B248, not 0x37B248 -- the `addiu` immediate is signed and
the `lui` carries the compensation. Chasing the un-compensated address
finds no symbol. (RSRC_info.)

**Retail clears and copies quadwords with bare `sq $0` / `lq $2`+`sq $2`
asm, not with C.** Any C spelling of a zero TImode store (`v.q = 0`,
`*(TI *)p = 0`, a `TI` local assigned 0) goes through `por rd,$0,$0`
followed by `sq rd` at a frame-pointer offset. The retail form is a
one-instruction `sq $0, 0x0(%0)` asm macro -- which ALSO forces the
destination address into its own register, reproducing the held-address
form that the offset spelling loses. Same for a 16-byte copy: an asm
`lq $2,0x0(%1)` / `sq $2,0x0(%0)` pair puts BOTH addresses in registers,
where a C struct assignment reaches the source with an `lq off(base)`.
Three nml functions needed this. A four-quadword clear is one macro with
0x0/0x10/0x20/0x30 offsets, not four single-quadword macros -- four
separate `"r"` operands become four separate `addiu`s.

**A one-instruction asm block still gets stolen for the `jr ra` delay
slot.** Wrap even a single `sq $0` in `.set noreorder` / `.set reorder`
when it is the last thing in a function, or the trailing `.p2align` pad
nop disappears and the function comes out one word short.

**The original ELF keeps every static symbol name, including
gp-relative small data.** `_gp` plus the symbol table turns a wall of
`sw zero,-14140(gp)` into named globals in one pass -- worth doing before
writing any global-heavy initialiser.

---

## Added from TMENU (drawDefault / updateDefault)

**The order of a store pair in ONE late block can pin a callee-saved
register channel for the whole function.** `TMENU_drawDefault` writes
`h04`/`h06` on nine different components from the same two locals
`x` and `y`. Which of the two gets `$s3` and which gets `$s2` --- and,
upstream of that, which float member is loaded into `$f1` --- is decided
by the store order in the *last* block that uses them, not by the
declaration order, not by the order the locals are computed, and not by
the first block that stores them. In that function eight blocks want
`h04` first and exactly one wants `h06` first; writing `h04` first
everywhere is "consistent", reads better, and inverts the entire
channel at a cost of twelve words. Sweep the store order of every
block that touches the pair, not just the first one, and let the diff
count pick --- it took drawDefault from 43 to 29.

**A named local read once is coalesced into its consumer's register.**
`n = t->b141; nOff = n * 24 + 12;` puts `n` and `nOff` in the same
register; retail has `move v1,v0` keeping them apart, which you only
get from the unnamed `nOff = t->b141 * 24 + 12;`. This is the mirror
image of the "repeated division must be a named local" row: introduce a
local to keep a value alive, remove one to let gcc *stop* sharing a
register. Six words in drawDefault, at two sites.

### Swept and unreachable here

**Respelling a `volatile` steering store does not move anything.** In
`TMENU_updateDefault`'s double `nFlags` store, fourteen distinct
spellings --- both stores volatile, the mask folded into the volatile
store, the mask as its own statement, the dead companion read moved
between the stores or respelled as `*(volatile int *)`, a volatile load
with plain stores --- emit **byte-identical** code. gcc normalises them
before scheduling. If a volatile-steered block has a register-naming
residue, the volatile is not the knob; stop respelling it. (Two
spellings do change things, but only by breaking the steering and
losing the store: volatile on the *pointee* rather than the pointer
object, and a volatile load with plain stores.)


---

## From the sound driver (sef/sdv/srsGet/ssd, wave 4)

**A record whose fields are reloaded before every single field store is
`volatile`, and nothing else reproduces it.** In the SGs* GIF packet
builders every store reloads both the write cursor and the packet base.
No aliasing story explains it -- a `sh` store forces the `int` cursor to
be re-read too -- and no statement order gets there. Typing the record's
fields volatile matched six functions at once. Look for this whenever a
tiny accessor re-reads the same two fields three times.

**A volatile store does NOT fold a byte offset into its store offset --
but a volatile STRUCT MEMBER does.** `*(volatile int *)(base + 4) = v`
costs an extra `addiu`; `((volatile QW *)base)->y = v` emits `sw v,4(r)`.
Indexing (`((volatile int *)base)[1]`) does not help either. So lay a
volatile struct over the target and name the member.

**`va_list` must be `__builtin_va_list`.** A `char *` typedef with
`__builtin_stdarg_start` compiles, but gcc spills the list pointer and
the function comes out one store long.

**A range test written `n >= 0 && n <= K` folds into one unsigned
compare.** Retail's `bltz` + `slt` pair only survives as two nested
`if`s. Worth checking on any post-call bounds check.

**A void call becomes retail's `j` sibling jump only as the function's
LAST statement, outside any else.** Inside `else { f(x); }` gcc emits
`jal` plus a shared epilogue -- four words more. Invert to `if (...) {
...; return; } f(x);`.

**PASSTHRU breaks the CSE that decides which of two equal values a test
reads.** After `nBest = n`, gcc knows they are equal and gives the
following `nBest == nNeed` test n's register; retail tests nBest's.
`PASSTHRU(nBest, n)` picks the other one and costs no instruction. This
is the only lever found for "same value, wrong register" after a copy.

**Type-based aliasing lets gcc hoist a `short` load above an `int`
store, and only a memory clobber stops it.** In a component-wise
fixed-point lerp gcc software-pipelines every component's `lh` above the
previous `sw`. Six `asm("":::"memory")` barriers reproduce retail's
order exactly -- recorded, not used, because that is more steering than
280 bytes is worth. If you meet this and the function is large enough to
justify it, the barriers do work.

**One local for a field's two roles.** Reading a record's magic word and
its size into the SAME local (retail keeps both in `$v1`) was worth 11
of 19 diffs in `smAlloc`; a dead loop cursor reused as the pointer to a
freshly split block was worth another 7.

### Build hazard, not a lever

**A block-scope `extern` declaration plus `-G8` is a time bomb.** gcc
2.9x keeps such a declaration's name on the function obstack and frees
it, then prints the dangling pointer in the `.extern` directive it emits
at end of file. It stays harmless while the freed memory happens to hold
a valid symbol name, and turns into `.extern ;, 4` -- which gas rejects,
failing the WHOLE file -- as soon as anything else in the file changes.
If a file suddenly stops assembling after an unrelated addition, look
for `extern` inside a function body and hoist it to file scope.

**Nested `if (n >= 0) { if (n < 3) ... }` instead of `n >= 0 && n < 3`.**
On a value loaded with `lbu`, the `&&` form folds to a single `bnez` --
gcc drops the sign test when the two comparisons are adjacent -- while
the nested form keeps the original's `bltz` + `slti` pair. Worth 30
diffs on xglCdStreamOpen and the same again on xglCdStreamClose.

**A tiny counted search loop must have its first iteration PEELED by
hand.** `i = 0; do { ...; i++; } while (i <= 0);` is a two-iteration
loop whose back edge gcc 2.96 proves dead and deletes, collapsing ~14
words. The original build kept both the peel and the (unreachable)
second iteration. Writing `if (a[0] == X) { a[0] = Y; } else { pp = a;
while (*pp != X) { i++; if (i > 0) goto done; pp++; } *pp = Y; }`
recovers the shape. Costs one pointer copy that the original does not
have -- still open.

**A bare `extern char` scalar is addressed through `$gp` at -G8.** When
the original uses an absolute `lui`/`sb` for a flag byte, declare the
object as an array bigger than the small-data threshold
(`extern char D_0093CC30[0x10];`) so it lands outside sdata. Same
trick as the padding already used to keep `mw` out of sdata.

**Two fixer passes were added for things no source shape reaches:**
`--short-loop-pad FUNC:N:COUNT` (gcc pads a short loop to a fixed TOTAL
-- 8 here, 10 in the original build; N counts ALL-NOP `.set noreorder`
blocks in the function, read off the POST-fix_cc_asm stream) and
`--byte-move-andi FUNC:N` (a register copy of a just-`lbu`'d byte where
the original zero-extends it explicitly). Both are provably
value-preserving by construction. `--short-loop-pad` closed
FileSelectListReload; `--byte-move-andi` closed xglMcSetMapName.
Note that the same function can want the copy in one loop and the
zero-extend in another, so the pass is site-keyed, not whole-function.

## Added from the tsk/screen state machines (tskUmn)

**Colour bytes patched into a string want the stores DUPLICATED in both
arms of the `if`.** Selecting the constant into one local and storing it
once after the merge makes gcc reach for `movz`; writing all three byte
stores inside each arm gives the branch pair, and gcc then cross-jumps
the arms back into retail's single store run. (Mail exwin greys its
"Reply"/"Set" captions by overwriting the `\014rgb` escape in place.)

**A run of byte stores emits as a LEFT-ROTATION BY TWO of its source
order.** Retail's `p[0]; p[2]; p[1]` needs `p[2]; p[1]; p[0]` in the
source. Same family as the store-group rotation already recorded, but
the rotation distance is per-block -- measure it, do not assume three.

**A local array of pointers with an INITIALISER LIST is what produces
retail's unaligned `ldl/ldr` block copy.** Spelling the same table as a
`static` template plus an explicit `memcpy` gives aligned `ld/sd`,
because gcc then knows both the source and the stack slot are 8-aligned.
The initialiser form keeps the type's own 4-byte alignment.

**A held pointer to a member costs seven words when the member is
already addressable.** Introducing `short *pX = &w->nUmlX;` so that the
loads through it act as an alias barrier does force the store/load order
-- but the declaration alone adds an address computation and six words
of fallout. Measured across all 16 use combinations in
`tskUmnDataBaseKeyWord`: every one is worse than reading the member.

**Where `w->msg.pText = ...` goes decides $s2/$s3 for the whole
function.** In `tskUmnMailInfo` the same store is shared after one
if/else and duplicated into both arms of another; getting the pair wrong
costs 47 words by renaming every callee-saved register from the dispatch
onward. When a tail store appears in some arms and not others, try both
placements per arm, not one policy for the function.


---

## From the character/collision sweep (Char, Hit, Undu, sub)

**Assign a built vector's X before its Y, even though the stores come out
Y-first.** Four corner points built as `c.y = ...; c.x = ...; c.z = ...`
compiled 15 words worse EACH than `c.x; c.y; c.z`, though gcc emits the Y
store first in both. Writing the source in the order the assembly shows is
exactly wrong here: 84 diffs to 5 in `HitCheckCorner`.

**A pointer read placed BETWEEN two struct copies decides whether its `lw`
lands before or after the copies' stores.** `pos = u->position; axis =
u->direction; bounds = u->bounds;` puts the `lw axis` in the middle of the
`ld`/`sd` block, where reading `axis` after both copies leaves it after the
last `sd`. Worth 4 words, and the last one fell to
`--rotate FUNC:N:5` -- a window rotation over four independent stores and
one independent load, which is the shape `--swap-adjacent` cannot express.

**Do NOT hoist a struct field into a local across a call.** `radius =
bounds.value.radius` before four calls makes gcc keep it in callee-saved
`$f20/$f21` and grows the frame 16 bytes; naming `bounds.value.radius` at
every use re-reads it from the local stack copy, which is what retail does.

**Declare a struct copy first but ASSIGN it later.** Declaration order picks
the stack SLOT, assignment order picks the COPY order, and retail wants them
opposite in this family: `HitAlignedBounds bounds;` declared before the
position (so it owns sp+0) and assigned after it (so the position is copied
first).

**Three separate `return 0` statements beat one wrapping `if`.** An early
`if (count == 0) return 0;` materialises the zero in the branch's delay slot
(`beqz s4,ret; move v0,zero`); wrapping the body in `if (count != 0) { ... }`
sends that case to the shared exit and costs 2 words. The same pattern --
retail materialising the return-0 at the TOP of the function and both early
exits sharing it -- shows up in `HitCheckBoxCorner` and
`UnduCheckSubHeightCheck`; a `result` local returned from every exit does
NOT reproduce it, it only adds a `move v0,result` at the tail.

**An unsigned switch index gives `sltiu` for the decision tree's range
test**, a signed one gives `slti`. One word, and it is a type fix, not a
tie-break.

**Compute a multi-term test into LOCALS when retail evaluates all the terms
before branching.** Three edge determinants written as the arms of a
short-circuit `&&` chain are each recomputed after the previous branch;
naming them `d1/d2/d3` first reproduces retail's one big FP block followed
by three compares.

### Axes that DON'T work (measured here)

- **The switch decision tree's PIVOT is not source-controllable.** ee-gcc
  balances a three-case tree at the MIDDLE case; retail's tree for the same
  three cases is rooted at the FIRST. Swept: splitting a merged range test,
  `LAUNDER_V` on per-store copies of the constant that shares the case
  label's register, a redundant extra empty case, and a nested
  `default: switch`. An if/else-if chain changes the arm LAYOUT (inline
  fallthrough instead of arms after the join) without touching the pivot.
- **A local array that never escapes will have its two loads CSEd.** Retail
  loads `*p` again at the top of a slide loop where gcc reuses the value the
  loop-bottom test loaded. `do/while` with an explicit guard, a plain
  `while`, and reading the index into a local before advancing the pointer
  all CSE identically.

---

## Wave-5 additions (src/xgl)

**A loop bound written INLINE in the `for` header is a different pseudo
from a named local.** jump.c's `duplicate_loop_exit_test` copies the
test ahead of the loop and LICM then hoists the invariant a SECOND
time, which is retail's `move t0,a2`; a named `nBytes`/`nLim` local
coalesces the two and the copy disappears. (xglFlagsGet, matched.)

**Independent conditional-move clamps are emitted in SOURCE order, and
that order picks the registers.** `xglCameraSetWindow` has two groups of
four `if (x < bound) x = ...` clamps; the emitted `slti`/`movn`/`movz`
sequence follows the statements exactly, so the group order decides
which of `$t1..$t4` and `$a2`/`$a3` each corner gets. Neither group is
in reading order -- the negative clamps run X0,X1,Y0,Y1 and the
screen-size clamps X0,Y0,X1,Y1. All 24 orderings of each group are
cheap to sweep and 14 diffs -> 4 -> match came out of it, where no
register lever moved anything.

**A PIN plus LAUNDER_V is how you BUY a `move` out of `$v0`.** When
retail copies a call's result (`move $v1,$v0`) before storing and
testing it, local_alloc coalesces the pseudo onto `$v0` in every source
shape. `PIN(int nRead, "$3"); nRead = f(); LAUNDER_V(nRead);` produces
the copy -- the bare PIN is silently ignored on a plain assignment, and
the LAUNDER_V is what makes it stick. The extra word then restores the
following `.p2align` pad by itself. (fillBuff, matched.)

**`--barrier-branch-move` already knows the volatile-store rule.** A
volatile MMIO store sitting immediately before a call is NOT stolen into
the call's delay slot by the original assembler; gcc's pipeline steals
it. The pass covers this (it looks for gcc's `#.set volatile` marker),
and it is worth trying on any function that mixes MMIO writes with
calls. An empty `__asm__ volatile` barrier in the C does NOT help --
gas fills the slot regardless. (xglRenderInit, matched.)

**An `extern char x[]` with NO size is not small data.** At -G8 gcc puts
a sized extern array in .sdata and reaches it with `addiu $a1,$gp,off`;
an unknown-size `extern char x[]` gets an absolute `lui`/`addiu` pair.
This is the mirror of the existing "declare it `extern char X[0x10]` for
the absolute lui" entry -- read the original's addressing first, then
pick the declaration. (xglCdLoadOverlay.)

**Write string copies as plain `(*pDst = *pSrc)` and delete the
steering.** Staging the character through an `int` and testing
`(c << 24) == 0` folds to `beqz` on an `lbu` value AND makes gcc comment
out its own `#nop` load-hazard pads (they appear as `#nop`, not a real
nop, once an `#APP` block sits between the load and its use). The plain
char-lvalue form keeps both the `sll 24` test and the pads. Three words
in xglCdLoadOverlay came back purely from removing a `PASSTHRU`.

**Spell `&sym` at its use rather than hoisting it into a local when the
original does not keep it in a callee-saved register.** A `void *gp =
&_gp;` local becomes a loop invariant that global_alloc ranks ahead of
the loop's own giv, and `$s6`/`$s7` come out the other way round;
writing `&_gp` at the single use took xglThreadInitial 15 diffs -> 10.

**A separate destination pointer per copy PHASE.** Two string-building
phases sharing one `char *p` make one pseudo, which takes one register
for both; retail uses `$a1` in the first and `$a2` in the second.
Splitting it also recovered the guard's branch-likely form. Took both
xglSoundLoadEffect and xglSoundLoadRequestSmd 13 diffs -> 7.

**Ranged `--swap-regs` composes, so it can express a 3-cycle.** A
permutation that is not a single transposition needs two swap sites over
the same window, applied in order; `xglHddMcCheckCore` needed
`10-11:50`, `3-10:52-55` and `11-3:52` to name its accumulator retail's
way. Reach for it only after the source sweep has bottomed out -- it is
assembly post-processing, so it costs a port nothing.
