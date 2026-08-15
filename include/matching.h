/*
 * Two kinds of assembly appear in this decompilation, and they have
 * opposite futures. This header names both so a port can tell them
 * apart mechanically instead of by reading every function.
 *
 *   1. STEERING constructs (PIN / LAUNDER / SCHED_NOP / PASSTHRU).
 *      These emit no machine code at all. They exist only to push
 *      ee-gcc 2.9x's register allocator and scheduler into the shape
 *      the original build produced -- a register tie-break that went
 *      the other way, a CSE that must not happen, a stall pad the
 *      original assembler inserted. They carry NO semantics. Build
 *      without MATCHING defined and every one of them disappears,
 *      leaving portable C that means exactly the same thing.
 *
 *   2. PS2_ASM blocks. The original really was assembly here: VU0
 *      macro-mode vector code, MMI 128-bit quadword moves, unaligned
 *      ldl/ldr pairs, kernel syscall stubs. These cannot be defined
 *      away, because they do the work. A port must supply an
 *      equivalent -- the macro exists so every such site is greppable
 *      and countable (tools/progress.py reports both totals).
 *
 * Rule of thumb when adding either: a steering construct needs a
 * one-line comment saying what the compiler did wrong. If assembly is
 * standing in for logic you could not express in C at all, that is not
 * a match worth keeping -- leave the function as a documented
 * near-miss instead, because the assembly is hiding something we do
 * not understand yet.
 */

#ifndef MATCHING_H
#define MATCHING_H

/* The byte-matching build defines MATCHING; ports leave it undefined. */
#ifndef MATCHING
#if defined(__mips__) || defined(__ee__)
#define MATCHING 1
#endif
#endif

#ifdef MATCHING

/* Pin a local to a specific hard register.
 *   PIN(int cnt, "$16");  ==  register int cnt __asm__("$16"); */
#define PIN(decl, reg) register decl __asm__(reg)

/* Make a value opaque to the optimizer in place: defeats CSE between
 * duplicate expressions, stops constant propagation folding a known
 * value, and acts as a scheduling fence at this point. Emits nothing. */
#define LAUNDER(x) __asm__("" : "+r"(x))
#define LAUNDER2(x, y) __asm__("" : "+r"(x), "+r"(y))
/* Volatile form: additionally pinned in place, so the optimizer may not
 * sink or delete it. NOT interchangeable with LAUNDER -- the two
 * produce different schedules. */
#define LAUNDER_V(x) __asm__ __volatile__("" : "+r"(x))

/* Compute src into dst's (usually pinned) register with zero emitted
 * instructions -- the tied empty-asm passthrough. */
#define PASSTHRU(dst, src) __asm__("" : "=r"(dst) : "0"(src))
#define PASSTHRU_V(dst, src) __asm__ __volatile__("" : "=r"(dst) : "0"(src))

/* An explicit stall pad the original assembler emitted and modern gas
 * does not. */
#define SCHED_NOP() __asm__ __volatile__("nop")

/* Faithful hardware assembly -- see note 2 above. Not removable. */
#define PS2_ASM(args...) __asm__ __volatile__(args)

#else /* portable build */

#define PIN(decl, reg) decl
#define LAUNDER(x) ((void) 0)
#define LAUNDER2(x, y) ((void) 0)
#define LAUNDER_V(x) ((void) 0)
#define PASSTHRU(dst, src) ((dst) = (src))
#define PASSTHRU_V(dst, src) ((dst) = (src))
#define SCHED_NOP() ((void) 0)
/* Deliberately left undefined: a port must replace each PS2_ASM site
 * with a platform equivalent, and should fail loudly rather than
 * silently drop vector/MMI work. */

#endif /* MATCHING */

#endif /* MATCHING_H */
