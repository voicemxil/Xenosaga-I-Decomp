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

**Read a byte global into an `int` local before comparing** — compared
directly, 2.96 knows the `lbu` result is 0..255 and narrows `slti` to
`sltiu`.

**Split a packed field into named locals.** `p->n >> 16` and
`p->n & 0xffff` inline makes combine narrow the high half into its own
`lhu` at +2 — two loads for one field.

**Struct-copy alignment picks the move idiom.** align 1 → all `lwl/lwr`;
align 4 → `ldl/ldr` for 8-byte chunks plus a `lw/sw` tail; align 8 (the
`union {...; long long ll[2];}` idiom) → `ld/sd`.

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

**Load-before-store source order is load-bearing** — gcc will not
reorder a store past a possibly-aliasing load.

**Do not cache a global across a call** — the original re-reads (it
must); caching costs a callee-saved register and a wider frame.

**Read a field back out after the `if`**, do not assign the local inside
it — fixes both the store's source register and gcc's jump threading of
a redundant re-test.

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

**A stale "near-miss" comment on a function that actually MATCHES is a
real hazard.** One cost an agent 30 minutes and briefly broke a
registered match, because the comment invited experimentation on
working code. `checkfile.py` is the authority, not the file comments.

**`set_flags.py` keys are BASENAMES, not paths.** Passing
`src/ssd/sef.c` used to write a key nothing ever reads, silently doing
nothing — three "the flag has no effect" cycles lost to it. The tool now
normalises and says so, but the fact matters when reading configure.py.

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
