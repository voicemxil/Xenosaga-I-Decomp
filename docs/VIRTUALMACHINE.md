# virtualMachine — the JVM bytecode interpreter

`0x002F1F08`, 9,280 bytes, 2,320 instructions. **0.7% of the entire ELF
in one function**, and the single most important one in the tree for a
port: Xenosaga's gameplay logic is Java bytecode, so this interpreter
plus the `Java_*` natives *is* the gameplay layer. Everything else is
presentation and platform glue.

Reconnaissance only — not attempted, not written. Recorded so whoever
opens it does not have to rediscover the shape.

## Structure

It is **not** one flat dispatch loop.

- **Prologue** handles native methods: `resolveNativeMethod`, then an
  indirect call, before entering the interpreter proper.
- **Main dispatch**: `lbu` the opcode, `sltiu op,202`, jump table at
  **0x4D2140** — 202 entries, i.e. JVM opcodes 0x00–0xC9.
- **Four secondary tables** at 0x4D2470 / 0x4D24E0 / 0x4D2550 /
  0x4D25C0. Each is 25 entries indexed by `op - 66`, and dispatches on a
  **halfword** (`lhu 0(s2)`), not a byte. These are the type-dispatch
  tables for the field and array access families.
- Only **26 `jal`s** in the whole body: `JNI_callMethod` (8x),
  `getField`, `getMethodSignatureClass`, `getClass`, `newArray`,
  `newObject`, `JNI_isInstanceOf`, `JNI_threadException`. Everything
  else is inline.
- **24 `lwc1`** — the float bytecodes are handled inline too.

## Register budget

`$s0`–`$s8` are all live across the body; frame is 160 bytes.

| reg | role |
|---|---|
| `$s3` | thread / frame |
| `$s6` | method |
| `$s2` | operand stack |
| `$s1` | locals |
| `$s7` | `$s2 + 8` |

## Advice for whoever opens it

**Five jump tables mean five `switch` statements**, and case-label order
decides block layout in each (see the switch entries in
`docs/LEVERS.md`, including that `balance_case_nodes` special-cases
exactly three nodes).

**The block-local-pointer lever is mandatory from the first line.**
`$s2`, `$s1` and `$s7` are function-scope temporaries reused across
dozens of arms — precisely the pattern that makes them multi-block
pseudos, which `local_alloc` skips, so `global_alloc` assigns them out
of `REG_ALLOC_ORDER` and wrecks allocation across the entire body. On a
2,320-instruction function that is unrecoverable if you get it wrong
early.

**Budget a multi-session effort.** This is not a one-sitting function,
and it should not be attempted by an agent that also has a byte quota —
the incentives are wrong. Give it its own run.
