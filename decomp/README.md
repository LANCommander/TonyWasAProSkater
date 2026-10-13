# THPS1 matching-decompilation harness

Matching-decomp arm of the project (the readable-C path). Toolchain runs natively on Windows via
Node — no Docker/WSL/Psy-Q-binary needed.

## What works now — full pipeline, all in Node (no Docker/WSL/binaries)

`C source → cc1psx (psyq-wasm) → ASPSX (psyq-asm) → R3000 words → diff vs SLUS_008.60`

- **Compiler:** `psyq-wasm` = PsyQ 4.4 `cc1psx` (GCC 2.8.1), byte-exact vs the real compiler.
- **Assembler:** `psyq-asm` = ASPSX 2.77/2.81-compatible encoder; expands macros and **inserts
  delay-slot / hazard nops** like Sony's assembler.
- **`tools/compile.mjs <src.c> [-O2] [-G0|-G8] [-o out.s]`**: C to assembly through the matchers' own pipeline (CPPPSX + native PsyQ 4.5 cc1, default `-G8`).
- **`tools/diff.mjs <src.c> <expected.txt>`** — instruction-level diff vs a Ghidra listing.
- **`tools/match.mjs <src.c> <retailAddrHex> <sizeBytes> [-G0|-G8]`** — the real deal: compile +
  assemble + compare machine words against the function's actual bytes in `SLUS_008.60`.

Validated on `CVector::operator+=` (`0x8004da3c`): the `move v0,a0` prologue and first load are
**byte-exact**, and psyq-asm inserts the correct delay-slot nops. The pipeline is complete.

## First target: `CVector::operator+=` (retail `0x8004da3c`) — 3/15 words, diagnosis clear

`src/vector.c` + `expected/CVector_apl.txt`. Run: `node tools/match.mjs src/vector.c 0x8004da3c 60 -G8`.
- Returning `*this` (operator+= returns a reference) made the `move v0,a0` prologue + first load
  byte-exact.
- Remaining diff = register allocation (`a2` vs `a0`) + software-pipelined load scheduling. Both are
  **compiler-version** artifacts: `psyq-wasm` is GCC 2.8.1 (PsyQ 4.4); the target schedules like
  **GCC 2.7.2 (PsyQ 4.0)**, standard for Aug-1999 titles.

## Workflow tools

- **`node tools/dis.mjs <addrHex> [size]`** disassembles a retail function for writing C. Call
  targets get their ported demo names, branches get `L<n>` labels, and gp-relative and `lui`-pair
  accesses are resolved to globals: the demo name from `data_names.tsv` (see below), else
  `DAT_xxxxxxxx`. Those are the names to put in `syms.json`. `m2c.mjs` uses the same names.
- **`node tools/m2c.mjs <addrHex> [size] [--asm]`** gives a **first-draft C decompilation** of a retail
  function via [m2c](https://github.com/matt-kempster/m2c). Clone it once with
  `git clone --depth 1 https://github.com/matt-kempster/m2c ../tools/vendor/m2c`; it's gitignored and
  vendors its own pycparser. The bridge emits GNU-as with labels, call names, `%hi/%lo`/`%gp_rel`
  symbol references and bare `div $zero,…`. GTE command words are labelled with their `INLINE_C.H`
  macro (`cop2: gte_sqr0`), and GTE register moves appear as `M2C_ERROR(ctc2 …)` markers. Use it for
  big functions: it gets control flow, switches and arithmetic right, then the README patterns fix
  the codegen details.
- **Diff views for `match_sym.mjs`:** `V=1` gives a full positional side-by-side. **`D=1`** gives an
  alignment-aware diff that LCS-aligns both instruction streams and prints only the differing hunks
  (`@@ mine 103 | retail 104`). Use it on big functions, where one shifted instruction makes the
  positional view useless.
- **Coverage tracker:** **`node tools/progress.mjs`** regenerates **[PROGRESS.md](PROGRESS.md)** with
  totals, history, coverage by size and by module, the closest near-misses and the biggest unmatched
  functions. It also appends to **`progress.csv`** whenever the numbers change. `salvage.py` runs it after
  every merge. `--backfill` rebuilds the history from git. Counting uses `functions.tsv` sizes.
- **`matched.tsv`** is the manifest of every byte-exact function: source, address, size, which
  matcher to use, and the mangled name. **`node tools/check_all.mjs [filter]`** re-verifies them all
  in parallel and exits non-zero on any regression. Run it after any tooling change.
- **`functions.tsv`** lists every retail *game* function (1,189 of them, 0x80010000–0x800858ac; the
  Psy-Q libraries follow, starting after `M3d_PreprocessPulsingColours`) with its size and its demo
  name or `FUN_xxxxxxxx`. Boundaries come from the static recompiler's control-flow analysis and
  agree with the demo SYM sizes for 472 of 474 named functions. `dis.mjs` and `salvage.py` take
  sizes from it, so unnamed functions work like named ones. `check_all.mjs` reports **coverage in
  game-code bytes** (481,452 total).
- **Names for `FUN_` rows:** callers often resolve a `FUN_` through a real name in their
  `.syms.json`. **`python ../tools/review_caller_names.py`** compares those names with the demo
  SYM and writes `caller_names_review.tsv`. Promote a name only after checking it, and rename the
  defining source, its header, `matched.tsv` and every caller in one step. Leave callers on the
  `FUN_` alias when the name isn't a C identifier (destructors such as `_._5CItem`). The same goes
  for C++ sources where the raw name would collide with one g++ emits itself (`__8CQuadBit` beside
  `struct CQuadBit`). `BASEF` is an alias some callers use for `_._4CBit` (0x80027e80).
- **`python ../tools/callsite_names.py`** is the stronger check. Exact twins have identical
  instruction streams, so the k-th `jal` in each calls the same function, and that gives the demo's
  name for each retail call target. Weigh it together with opcode similarity and source-order
  adjacency (same `.CPP`, same order). Byte-shape matching alone mislabels identical bodies:
  `0x800241dc` was `__nw__5CItemUi` but is `__nw__6CClassUi`, and `0x8006e19c` was `Mem_Delete`
  but is a panel wrapper.
- **Source order settles most of the rest.** `python ../tools/align_demo.py <lo> <hi> FILE.CPP[,…]`
  lines a retail range up against a demo file's function order, and `../tools/slot_check.py` scores
  proposed names. Many classes share a destructor or constructor shape, so a "100%" match can be
  the wrong class. When the slot and the twin disagree, compare the `jal` targets and the vtable.
  That's how `0x800275ec` (`_._4CCar` → `_._6CGLine`) and `0x80029d80` (`__9VMenuItemi` →
  `__10CRibbonBit`) were fixed. Destructor sources keep a `FUN_`/alias function name in C. Their
  header comment carries the real mangled name.
- **Use class layouts for THPS1-only code.** `MAIN.SYM` defines classes the demo never calls,
  and `python ../tools/sym_locals.py --struct NAME` prints one with its field offsets. When a
  function's field accesses match a layout, the class is proven even without a demo function.
  That's how invented `SaveRect` and `MenuInput` turned out to be `EraseSprite` and `PadBuffer`.
- **`inferred_names.tsv`** lists every name that isn't a demo function name verbatim: SYM class
  with a descriptive method name, a slot-only name, or a changed signature. Each row records its
  basis. Add to it whenever you name something the SYM doesn't name.
- **Globals get their names from demo twins too.** `python ../tools/data_names.py` aligns each
  twin with its demo function by opcode and pairs up their `lui`/`%lo` and gp-relative accesses. The
  demo SYM names the demo address, and each pair votes for that name at the retail address. It
  writes **`data_names.tsv`**: address, name, votes, demo address and C type. `exact` rows are a
  demo global. `offset` rows are `<Base>_<hex offset>` inside a typed demo object, and the note
  gives the demo member. A name needs 75% of its address's votes, and only matching runs of at
  least 4 words in twins at least half aligned count (shorter runs and worse twins disagreed with
  the confident votes most of the time). Each demo global keeps only its best-supported retail
  address. `1/1` rows rest on a single access, so check those first if something looks off.
  Two more passes cover what no twin reaches. `layout`: the data sections keep each module's
  variable order, so between two exact names the same distance apart in demo and retail, every
  demo symbol keeps its offset. A leave-one-out test on the exact names got 294/294 right. With only
  one neighbour, the same guess was wrong 44 times out of 113, so that isn't used. `string`: a
  referenced address that holds a C string becomes `s_<text>`, the way Ghidra names them.
  Static members (`_7CCamera.TweakCol`) become `CCamera_TweakCol`, and function-local statics
  (`old_camDist.394`) drop the counter.
  **`python tools/rename_data.py [--apply]`** then rewrites `DAT_` in the sources and the keys in
  their `.syms.json`. The address and type stay, so the code doesn't change. A file where the name
  is already an identifier (a member declaration, local or type) keeps its `DAT_`, because the new
  name could bind to that instead. Comments and `x.f`/`p->f` accesses don't count, and neither
  do members of a plain struct (C has no methods; in C++, no `(` in the body and nothing derives
  from it). If a placeholder type or typedef takes the name, rename it to its SYM tag (`Pad` →
  `SControl`, the skater record `SSkater` → `SStats`). **`data_names_manual.tsv`** holds the
  globals named by hand, each with its basis (e.g. `GBoardStats`: retail moved it out of
  `.sdata`, so the demo twin's gp access doesn't align). `data_names.py` merges it in. Verify a rename with `compile_all.mjs` before and after, with the names mapped
  back to `DAT_`, plus `check_all.mjs`. Rerun both scripts as more functions get twins. The
  rest of the `DAT_`s are THPS1-only, or no twin touches them.
- `src/unnamed/<addr>.c` holds unnamed functions. The trivial ones (empty bodies, `return K`,
  one-store setters) were generated mechanically from their bytes.
- All matchers use per-process temp files under `build/`, so they can run concurrently.
- **`python tools/salvage.py [--all|--dry-run|paths]`** verifies candidate sources (by default the
  untracked/modified ones under `src/`). Each needs a first-line header `/* <mangledName> (0x<addr>) */`.
  It tries div → sym → cc, appends EXACT rows to `matched.tsv`, and regenerates **`nearmiss.tsv`**
  (≥85%, with scores). Grinding agents write sources, and this merges them.
- **`D=1` for `match_div.py`** too: the same disassembled, LCS-aligned hunk diff as `match_sym.mjs`
  (via `tools/wdiff.mjs`).

## Names as written vs. linker symbols

Names like `Mess_SetTextJustify__FUc` are g++ 2.x **mangled** symbols: what the compiler made of
`Mess_SetTextJustify(unsigned char)`. They're the unique key everywhere (`functions.tsv`,
`matched.tsv`, `*.syms.json`) because overloads share a written name.

- **`python tools/demangle.py NAME…`** demangles. It wraps the GNU v2 demangler Ghidra ships;
  modern `c++filt` dropped the scheme. `python tools/demangle.py --tsv` refreshes `functions.tsv`
  column 5 ("as written"), which `dis.mjs` and `PROGRESS.md` show. Run it after renaming.
- **C sources use the written names** via `include/names.h`, generated by
  `python tools/gen_names_h.py [--apply]`. Each line is `#define Mess_SetTextJustify
  Mess_SetTextJustify__FUc`, so the source reads `Mess_SetTextJustify(0)` but the compiler still
  emits the mangled symbol. `.syms.json` keys stay mangled.
  - Free functions keep their name. Methods become `Class_Method` (`CBruce_Vibrate`), and
    constructors `Class_ctor`.
  - Overloads, operators, `new`/`delete`, destructors and names that clash stay mangled (43 today).
  - `--apply` rewrites the sources and adds `#include "names.h"` as line 2. It never touches line 1
    (the salvage header) or string literals, and it's idempotent: rerun it after adding sources or
    renaming. `-I include` is on the preprocessor command line in `psyq.mjs` and `match_div.py`.
  - `.cpp` sources don't include it; they declare real C++ prototypes. New C sources can call
    functions by their written name and include `names.h`.
- **Check source-wide edits with `node tools/compile_all.mjs <tag>`.** It compiles every `.c`
  and `.cpp` under a tag, so you can compile before and after and diff the assembly. The names.h rewrite
  changed nothing except each defining file's own label, which became the mangled symbol (more
  faithful).

## Hand-written assembly modules (`.s` sources)

A few game modules were written in assembly, not C. You can tell from non-cc1 idioms: `addi`/`ori`
for constants, `bgez zero` branches, frameless leaf code, and the GTE LZCS/LZCR used as a
leading-zero counter. For those the faithful source is a `.s` file:

- **`node tools/asm2s.mjs <addr> <size> <out.s> [name]`** generates one from retail. Every
  `functions.tsv` entry in the range becomes a global label, branches get local labels, external
  calls use their ported names, and the file runs under `.set noreorder`. The first line is a `#`
  header that salvage reads: `# <name> (0x<addr>) size=<n> -- hand-written assembly`.
- **The matchers take `.s` directly:** `cc1()` in `tools/psyq.mjs` passes it to the assembler
  unchanged. salvage, check_all and progress treat it like any other source, so one `.s` row with
  `size=` covers every function inside it.
- **Data embedded in `.text`** can go in the same module as `.word`s with a label. Example: the
  memory-layout table at 0x80071fa4, split out of `FUN_80071f84` in `functions.tsv`.
- **Names for asm entry points:** `MAIN.SYM` has no function records for asm, only bare labels.
  `../tools/match_functions.py` ports those that match retail exactly as `label_matches` in
  `build/matches.json`, and they reach `retail_names.tsv` as `D` rows. Check there before you leave
  an asm label as `FUN_`. Callers use the raw (mangled) label with C linkage: `extern "C"` in C++.
- **Modules so far:**
  - `src/shatter/mdecdecode.s` (0x80022498, 1364 B): MDEC run-length/VLC decode, 10 entry points.
    The demo names the first two `DecDCTvlcBuild3`/`DecDCTvlc3`, so this is probably a Psy-Q
    libpress variant linked into the game.
  - `src/init/fixedmath.s` (0x80071fa4, 448 B): the layout table plus the 64-bit fixed-point
    helpers `Mips_DotProd2Sign`, `Mips_FixMul__Fii`, `Mips_DotProd2` and
    `Mips_CVecDotProd__FP7CVectorT0` (a CVector dot product, >> 12).

## Data (`data/`)

The initialised globals (retail `.rodata`, `.data`, `.sdata`; 0x8009d0e0-0x800d21c8, plus the
table in `.text` at 0x80071fa4 and the free-memory pointer after `.bss` at 0x800dc94c) are C++ in
`data/<module>.cpp`, one file per `src/` module that uses them (C++, as Neversoft's sources were,
so they can use the game's types from `include/game`). Each global carries its retail address,
`PSX_DATA(addr)` (`data/psxdata.h`), under the name the decompiled code uses (its `.syms.json`),
else `data_names.tsv`, else `D_<addr>`.

**No globals for text.** Text is a string literal where it's used: in the code
(`Font_String(x, y, "Follow", ...)`) and in the tables, which have the game's types
(`SLevel Levels[10] = {{"Warehouse", "Woodland Hills", "ware", ...}, ...}`, `char *AnimNames[6]`).
The only `char` arrays left are buffers the game writes to (`SaveGameName`, `CARD_FNAME`). Other
pointers name their targets (`(SFoo *)D_800a1234`, a function's name in a vtable). Tables nothing
types yet are still word arrays, one element per group of lines with the retail type in a comment
(`/* THPS2 demo: CTape*[5] */`). **`python tools/type_data.py Name 'Type' [--include game/x.h]`**
types one: it reads the table's bytes as they are and the type's layout from the DWARF of a probe
compiled with the headers, and rewrites the definition as a typed initialiser, string literals
for the text it points at (`type_data.py Stats 'SStats[11]' --include game/career.h`). It can't
change the data: the check below compares the result with the retail bytes.

These files are compiled only for the port (the matching build declares what it uses itself), and
in the port they're ordinary native data: `platform/pc/tools/data_image.py` lays the globals out
in their retail layout (the decompiled code relies on it: a struct it names as several globals,
an index from one table into the next) as `gen/data.s`, with native pointers and literals, and
the code's references by retail address (`psx_<addr>`) link to them. Only `data/fixed/`
(`PSX_FIXED`: the vtables, whose addresses the port stores as numbers, and `DoubleBuffer`, whose
GPU packets have 24-bit links) stays in PS1 RAM. `data_image.py --check` compares every global,
laid out at its retail address, with `SLUS_008.60`: equal bytes, a pointer to a literal where
retail points at the same text, and retail text no global holds any more must be a literal in the
data or in `src/`. Left out: the code and `data/omitted.tsv` (Psy-Q library data the port doesn't
use, MIPS switch jump tables, the memory card icons, debug text no code in the port prints). The
port build runs that check and fails if a data file stops matching, or if decompiled code names a
global the image leaves out.

The matchers place a TU's string literals at the retail addresses the code names, piece by piece
(`tools/psyq.mjs` `link`, `tools/match_div.py`), with retail's bytes there checked against the
literal; a one-function source's literals match wherever retail's whole-file TU put them, so the
source writes `"Follow"`, not `extern char s_Follow[]`. A load of a constant string's bytes is an
inlined copy: `strcpy(p->name, "")` (`8006613c.c`). egcs merges equal literals in a TU, so two
retail strings with the same text that one function uses can't both be literals: they're buffers
(`s_800a42dc`/`s_800a42e8`, the saved names; `DAT_800d1138` in `memcarddisplay.c`).

`tools/gen_data.py` generated the first version of the files (from `SLUS_008.60`, the names, the
`.syms.json` references, and the struct sizes of a `platform/pc/tools/layouts.py` snapshot,
`--layouts`): word arrays, text as globals. They have been reworked by hand since (native, typed,
literals) and are edited by hand from now on; don't regenerate them. They keep their own layout:
`data/.clang-format` turns clang-format off there.

## Demo twins: start here for any named function

`build/matches.json` lists the retail functions that are byte-identical to a THPS2-demo function once
relocations and load/store offsets are masked (field offsets can differ: THPS1's `CBody` is 0x2c
bytes bigger, for example). **77 of them were still unmatched (45.8 KB)** when this was found. The
demo's `MAIN.SYM` holds each one's full debug info, and that info decides most of the codegen
choices that blind iteration can't find:

- **`python ../tools/sym_locals.py <name>`** prints the function's parameters, nested blocks and every
  local: its exact C type, its storage class, and the register or stack slot cc1 gave it, in
  declaration order. Write the C with those variables, those types and those scopes.
  `--struct <Tag>` prints a struct/union layout from the SYM's 157k type records. `--lines` prints
  the source line that starts at each word, which splits the retail listing into the original statements.
- **`python ../tools/twin_calls.py <retailAddr> <demoName>`** names every `jal` target from the
  twin's call at the same word, including Psy-Q and `M3dMaths_*` library calls retail leaves as
  `FUN_`. That's half a `syms.json`.
- **Write it as C++** (`src/<module>/<name>.cpp`), since Neversoft's code is C++. A `.cpp` source goes
  through `CC1PLPSX.EXE` with `-fno-exceptions` automatically, in every matcher and in
  `check_all`/`salvage`. Declare `CVector operator-(const CVector&, const CVector&);` and so on, and the
  calls, `const int&` temporaries and by-value returns come out the way retail has them. Wrap SDK
  headers in `extern "C" { }`. Globals the source *defines* land in `.sdata`. Both linkers resolve them
  through their own `syms.json` entries, so no section base is needed.

Results so far: LineLineClosestPoints (1964 B) matched on the 2nd compile. RenderEllipsoid (1940),
PutDataChunk (1956), CTape::AI (1656), Mess_Display, Load8BitBMP2 and Trig_ParseTRGFile matched
within a few iterations each. The last three had been stuck at 97–99% as C.

What the SYM fixed, by function:

- **Intermediate variables.** ParseTRGFile had `char *pNodeName = … + 6;` before the post-increment
  store. Mess_Display stores the scale straight into the global and passes the global. PutDataChunk's
  `PredX/PredY/PredZ` caused retail's odd spill.
- **A shared stack slot.** Block-scoped `CVector Dummy` and `Pos` at the same offset means one variable.
- **Hidden arguments.** In Load8BitBMP2, `bpp` was `printf`'s second argument
  (`"Not 8 bits per pixel, is %d bits per pixel"`).
- **Declaration position.** CTape::AI's `int message_count = 0;` sits *after* the interpolation block.
  Initialising it at the top cost a callee-saved register.
- **`volatile`.** PutDataChunk's vblank counter is re-read each time.

**More patterns from the S18 grind, each verified on retail:**

- **Constants inside both ternary arms.** Write `((c1 ? (c2 ? a : b) : d) << 12) * 3 * 15 / 28` as a
  single expression. gcc then folds the constant into each arm, with a `lui` in the delay slot and an
  `ori` at the head of each arm, as retail does.
- **16-byte stack vectors.** `typedef struct{int x,y,z;} __attribute__((aligned(8))) V;` puts vector
  locals 16 bytes apart. Copy through a plain 12-byte type, or the copy becomes 4 words.
- **`slt; xori; beq`** comes from `m = (a >= b); if (m) …`.
- **6-byte by-value args.** `SV sv = *(SV*)&field; f(sv);` gives the `lwl/lwr` temp plus the `lhu` argument loads.
- **Keeping identical arm tails apart.** When two arms end in the same code, crossjump merges them.
  Give each stage a fresh variable to keep the tails distinct.

Residues these don't explain are local-alloc ties. **The register that `a + b` reuses is the first
operand's.** `&tab[i]` canonicalises to `base + i*4`, so when retail ties the sum to the shifted index,
write `(T *)((i << 2) + (int)base)`. That fixed RenderEllipsoid's `addPrim` and CTape::AI's
`&mMessageTable[n]`.

**More from the S19 grind, each verified on retail:**

- **Callee return types change register allocation.** A call to a function that returns a value is a
  `call_value`, which sets `v0` even when the result is ignored. Declaring a callee `void` when it
  really returns something shifts local-alloc's choices around every call. CCar::AI (1300 B) went from
  322/325 to exact by declaring `Trig_GetPosition` as returning a pointer, and CMenu::Update (904 B)
  from 219/226 by making `SFX_PlayX` return `int`. Check the callee's epilogue for a `v0` write.
  **`tools/retsweep.py`** tries `void`→`int` on every prototype of every near-miss source.
- **Compare-tree switches: fit the case set with `tools/casetree.py`.** It emulates GCC 2.8's
  `group_case_nodes`/`balance_case_nodes`/`emit_case_nodes`, plus the jump optimizer deleting a
  compare whose target matches the jump after it, and checks the predicted compare sequence against
  retail's. Empty `return x;` groups and a case that shares the `default:` label are invisible in the
  bodies but shift the tree's balance. SkipCommand (808 B) needed three such groups, found by searching
  over optional nodes in the value gaps (`casetree.py --s` checks the model against your own build).
- **Shared case tails: write the `goto`.** Two cases whose bodies end the same way get merged by
  cross-jumping, but that happens after register allocation, so duplicated source doubles the shared
  variables' ref counts and changes their registers. `case 1: f(104); goto Bouncy; case 3: f(40);
  Bouncy: …` fixed CPlatform::Bounce's `s2`/`s3` swap.
- **A const global hoisted out of a loop through gp:** declare `extern const int G;` before the
  function and define `extern const int G = 0;` after it. The function body can't fold the value,
  `const` (RTX_UNCHANGING) lets loop.c hoist the load into the preheader, and the definition makes it
  small data, so the load is `lw rX,N(gp)`. A plain global stays in the loop, and a `const` defined
  before use folds to its initializer.
- **Two-statement byte reads:** `v = *p++; v += *p++ << 8;`. Retail increments between the loads, and
  `*p++ + (*p++ << 8)` makes gcc load the same byte twice. The line table shows two lines.
- **Remainders spelled out:** `remx = width - numx * slice_width` gives retail's `mult`+`subu`;
  `width % slice_width` gives `mfhi`. `(n - 1) - a * b` vs `n - (a * b + 1)` changes CSE as well.
- **A variable that dies twice isn't local.** Local-alloc only takes pseudos with exactly one death.
  Reassigning a variable from its own value across several insns (`r = (r << 12) / d;`) gives it two
  deaths, so global-alloc places it instead, usually in a worse register. Use a fresh variable for the
  new value (InitLineInfo's `short Cos2`).
- **The SYM omits some variables.** InitLineInfo's cosines, pack's `numx` and ParsePSX's hoisted
  shift aren't listed, but retail keeps them in registers. If retail holds a value no listed local
  explains, add a local for it.
- **`gte_nop()`** pairs are real source statements: two `nop`s between `gte_ldlzc` and `gte_stlzc`.
- **Constructors as real C++ classes.** Declare the hierarchy with the first polymorphic class's data
  members before its virtual functions, so g++ 2.x puts the vptr after them (CBit at 0x44, CItem at
  0x48). The derived constructor then stores `_vt.<len><Class>`; put that symbol in `syms.json` at the
  vtable's retail address. Members that THPS1 moved can be padded to retail offsets
  (`char pad4c[0x6c - 0x4c];`). The CSmokeJet and CTape constructors matched this way.
- **Mixed running pointer vs. indexed array in a loop:** `mAmp[i] = i ? Length / 80 : 0;` gives a
  running pointer for `mAmp`, where an `if/else` with two stores indexes from `this`.
- **Assigning a parameter moves its prologue copy.** The `move sN,aN` copies are scheduled with
  the register saves, and a parameter the body writes to gets its copy earlier. If retail copies
  `this` before an argument but your build doesn't, stop assigning that argument and use a temporary
  (`int t = -vel; …`), or compute into the member (`mNumSections = NumPoints * 2;` and then use
  `mNumSections`). That fixed the CGlow and CGrindSpark constructors. Writing a constructor as a real
  C++ constructor fixed the CPowerUp one.
- **`new T` with a null check** before the constructor call means the class has its own
  `void *operator new(unsigned int) throw();` (CBit's pool allocator, `__nw__4CBitUi`).
- **A narrow stack argument read late:** pass the parameter straight to the narrower callee
  (`FUN(..., scale)` with a `ushort` parameter). gcc then never copies it at entry, and loop.c hoists
  the `lhu` into the loop preheader as retail does.
- **ADDRESSOF variables are re-read after every store.** A local whose address goes to inline asm
  (`gte_stlvnl2(&zed)`) is an ADDRESSOF pseudo during CSE, and after it is purged its address is based
  on a fresh `$1` (soft frame pointer) rtx that `rtx_varies_p` treats as varying. Every later store
  then invalidates it. If retail reads such a variable once, the source used it once: compute the
  derived value into a local (`int idx = (zed >> 2) & 4095;`). The same effect blocks
  `SlicedImage::pack`'s `dum2`.
- **`OPS=1`** with `D=1` (match_div, via `tools/wdiff.mjs`) aligns on opcodes and immediates only, so
  register noise drops out and only the structural differences are printed. **`DEMO=1 node
  tools/dis.mjs <demoName>`** disassembles the THPS2 demo's version of a function for comparison.
- **`ABS(x)` is a macro:** `(((x) ^ ((x) >> 31)) - ((x) >> 31))`. `abs()` gives a branch.
  `ABS(a) > ABS(b)` loads `a` first, while `ABS(b) < ABS(a)` loads `b` first.
- **Chained assignment stores right to left:** `p.a1 = p.a3 = p.a4 = p.a2 = 0;` emits the stores in
  the order a2, a4, a3, a1, which separate statements never produce (Crowd_Start). Read the store
  order off retail and write the chain backwards.
- **Two array elements loaded with separate `lui`s are two symbols.** If retail loads `X[0].f` and
  `X[1].f` each with its own `lui`/`lw %lo`, declare them as two externs (`DAT_800bf318`,
  `DAT_800bf3d8`). Indexing one array lets CSE share the base address (Card_Write).
- **Store through `reg+off` with the base in a fresh register = an inline member.** Retail's
  `lui a1; addiu a1,%lo(X); sh a0,2094(a1)` comes from `X.SetTime(v)`, where `this` is a pseudo that
  CSE later reuses. `X.mTime = v` folds the offset into the symbol instead.
- **Sin table as a struct:** `struct { short s, c; } T[]; T[a & 4095].s` (or an `int[]` cast to
  `short`) allocates registers differently from `((short *)T)[(a & 4095) * 2]` (CMedal::DoPhysics).
- **Locals the SYM lists as `auto` that retail reaches through `t0`** (`addiu t0,zero,640;
  sw t0,16(sp)`) were spilled by global-alloc. They are not `volatile` and their address isn't
  taken; the reload register `t0` gives them away. Fix the allocation by matching statement order
  and every reference, e.g. a store that CSE folded into one pseudo's address (Card_Write).
- **Load-after-store ordering is not aliasing:** a global load scheduled above a member store in
  retail but not in yours usually means a different statement order (`f234 = Time; f230 = -1;`) or
  an intermediate `base` local (`unsigned *base = Tab; p = base + i;` fixed CamPt's register pair).
  A store of a call result straight from `v0` is grouped with the call by the scheduler, so nothing
  can be placed between them.
- **A global load that retail hoists above member stores → `extern const int G;`.** If retail reads a
  scalar global (e.g. the level index for an `sprintf`) before two `sw zero,off(sN)` member stores
  and yours reads it after, declaring the global `const` (RTX_UNCHANGING) lets the scheduler hoist it.
  This made Competition_ResetMarquee exact and fixed the GameTime load in CScriptOnlyBaddy's
  ExecuteCommand. It does not help when the stalled load is a *member* sitting behind a *global*
  store. For that case, read the member into a local *before* the global store
  (`unsigned snd = mLoopingSound; DAT = …; SFX_ModifyPos(snd, …)`), which made CScriptOnlyBaddy::AI
  exact. Likewise `int n = G; p->h++; G = n + 1;` for a global increment that retail loads before a
  member store (Video_Playback).
- **A value reloaded just before a join, ahead of a global store:** keep it in one local that both
  paths set: `int st = p->m734; if (st == 4 && …) { …calls…; st = p->m734; G = 1; } if (st || …)`.
  The local is dead across the calls, so it stays in a caller-saved register. The reload lands
  before `G = 1` instead of after the join. This made Video_Playback exact.
- **Arguments set up *after* unrelated stores → `do { call(); } while (0);`.** Loop notes are
  scheduling barriers, so a call wrapped in a one-pass loop (probably a macro in the original) keeps
  its `addiu a0/a1` argument setup after the preceding stores. Retail's line table showed a new scope
  starting exactly at the argument setup. That fixed CCamera::ReactToCollision's ApplyMatrixLV block.
- **Unfillable delay slot after a multiply:** cc1 leaves a branch in `.set reorder` for ASPSX, which
  never fills it, when reorg thinks LO is live at the branch target. Reorg scans linearly from the
  previous barrier, and the reload-generated `mflo` (mulsi3's output is `=l`) carries no REG_DEAD
  note, so LO stays "live". A call (which clobbers LO) before the branch makes the fill happen. It is
  still open how retail's ReactToCollision got its `beq …; mult a1,s0`.
- **Delay slot stolen from the branch target ↔ the polarity of the test.** Retail's `beq v0,zero,L;
  addiu s3,zero,-1` (an instruction copied from L, with the branch redirected past it) came from
  `if (!LOS(...)) Index = -1; else { …success… }`. The positive form `if (LOS(...)) {…} else
  Index = -1;` filled the slot from the fall-through instead. This made CamPt_SetClosestCamera exact.
- **Seeing cc1's view:** `CC1PLPSX.EXE x.i -quiet -O2 -fno-exceptions -G8 -dd -o x.s` writes `x.i.dbr`
  (RTL after delayed-branch scheduling, with REG_DEAD/REG_UNUSED notes and live sets). Make `x.i`
  with the CPPPSX line from `tools/match_div.py`.
- **Struct-returning operators written into the destination:** `a = a + b` in C++ builds a temp and
  copies it. Retail's direct-to-destination shape comes from calling the hidden-return operator as
  `extern "C" void alias(T *ret, ...)`, with the alias mapped to the real address in syms.json and
  explicit `int` locals for `const int &` arguments (Shatter_Glass, from 113 to 181 aligned words).
- **CQuat derives from a polymorphic CClass (vptr at +0).** `q = r` is a 5-word copy that re-stores the
  first word last, which accounts for retail's odd `lw v1,vptr; … sw v1,vptr` tail.
- **Five consecutive gp-relative shorts are five separate `short` globals.** A `short[5]` is 10 bytes,
  over the `-G8` limit, so it gets `lui` addressing.
- **Initialise a loop counter after the call before the loop** (`slot = Rnd(); j = 0;`). Initialising
  it before the call keeps it live across the call, which forces an s-register.
- **Use the `_x,_y,_z` inline setter for every stack vector, not just the first.** Plain field stores
  on the later ones shift the stores and registers (CChunkBit ctor).
- **Compare against the value you clamp *from*:** `if (Raw > 2400) Dist = 2400; else Dist = Raw;` puts
  the call result in its own register and the clamped copy in another. `Dist = Raw; if (Dist > ...)`
  merges them.
- **`sll tmp,n,3` before the branch, with `addu v,v,tmp` in its delay slot (in an `a3`/`v1` temp):**
  the pointer advance was written *before* the `if`: `int r = Clip(v, n); v += n; if (!(r & mask))
  { n = p->m; w = v + p->i; …; Draw(w, v, n); }`. Reusing `n` for the next count and computing `w`
  before the inner call give retail's s-register assignment. This made the six M3D render wrappers
  (0x80084d18–0x800850c4) exact.
- **A loop that keeps its entry `j` to the test (not rotated) is a goto loop.** cc1 rotates every
  `while`/`for`, and loop.c then strength-reduces it. Retail loops with `j test` at entry and no
  strength reduction (`buf+i` recomputed in the body) come from `i = 0; goto test; body: …; i++;
  test: if (i < N) goto body;` (FUN_8006ed2c exact; FUN_8004b0e4 at 35/37). An increment in the
  back branch's delay slot is `test: if (j < N) { j++; goto body; }`. No -O/-f flag reproduces it.
- **Several exits sharing one `return r` epilogue (`addu v0,v1,zero` after the restores):** write
  `goto out;` for each exit and put `out: return r;` after the loop. `if (r == -1) return r;`
  constant-folds to `return -1`, and `break` lets cc1 rotate the `for (;;)` (FUN_800604c8). A
  two-case switch after a separate `if` gives retail's linear `beq; beq; j default` chain. A
  three-case switch builds a compare tree instead.
- **An `if (G) x = f(a); else x = f(a);` pair is cross-jumped into one call.** That leaves a
  vestigial `bne …; addu a0,s0,zero` in front of it (FUN_80027d08).
- **Static object construction/teardown (`_GLOBAL_.I/.D`) comes as a pair: one source, `size=` both.**
  Defining `X G;` in a file emits D then I, back to back (FUN_80077060, size=196). An *implicit*
  destructor of a polymorphic class never re-stores its own vptr; only inlined base dtors' stores
  remain. A user-written `~X() {}` does store it. Read the retail vtables (`.word` dtor entries)
  to tie destructors to classes.
- **`ot` local for `addPrim`:** `{ u_long *ot = DAT_800d1a48 + 39; addPrim(ot, p); }` gives retail's
  register choice. Passing `DAT_800d1a48 + 39` straight into the macro doesn't (FUN_80056394).
- **Two globals that are elements of one array get separate `lui` bases:** the second element's
  fields at offset > 0x7fff go through a separate symbol (`extern T DAT_800b09c8;`) while
  `DAT_800a891c[1].on` uses the array (FUN_80048c8c).
- **Some functions need `-fstrict-aliasing` (header note `flags=-fstrict-aliasing`).** The symptom is
  a load from a struct field (an `int`) scheduled above a store to a pointer global, which plain
  `-O2` won't do. FUN_80048fec, FUN_8006b01c and FUN_8006b434 are exact only with it. Over all of
  matched.tsv, 896 functions match either way and 6 *break* with it (adv, setradius, 80061ef4,
  load8bitbmp2, ribbon_ctor, 8006c4f0), so it's per-TU, not global. `check_all.mjs` and `salvage.py`
  read `flags=a,b` from the header line (sym: appended to `-O2`; div: `CC1_OPTS`). Try it on a near-miss
  whose only residue is load/store order. Another tell (M3d_RenderBackground): after a store to an
  `int` global, a `uchar` field is *reloaded* (char aliases everything) while a `ushort` field load
  moves freely above the store (TBAA says it can't alias).
- **Pointer advance with the index first (`addu v0,idx,base`):** g++ always emits `p + i` as base
  first, whatever the C order. Retail's index-first form comes from integer arithmetic:
  `p = (uint *)(n * 4 + (int)p) + 1;` (M3d_PreprocessPulsingColours).
- **A byte read twice with no reload (`lbu t1` for a difference, then `addiu v0,t1,-1; sb` for
  `x--`):** egcs CSE can serve the narrow QImode load in `x--` from the earlier widening
  `(zero_extend (mem:QI))`, but only if *no store* comes between them. A zero_extend table entry
  isn't a MEM, so every store invalidates it. Move the stores (here the UV word copies) above the
  reads (Font_Display). A difference in register pressure like this one moves every allocation in
  a big function, so check it before chasing register names.
- **Expression shape picks which invariant gets hoisted:** `LetterY + 2 + Height` and `LetterY +
  (Height + 2)` compile differently once `LetterY + 2` is a loop invariant. Try every association
  (Font_Display needed a different one on each of two near-identical branches).
- **`-fstructure-noalias` (header note `flags=-fstructure-noalias`): the big one.** Our egcs
  1.1.2 cc1 normally treats every store through a pointer as possibly aliasing every global, so a
  global load never moves above a struct-field store. Retail moves it. `-fstructure-noalias` (an
  option of this cc1; `strings` turns it up) lets it, and makes 14 near-misses exact: CSpark ctor,
  CMenu::Display, LoadIntoMikeCamera, Font_String/Font_Number, DoShadowCheck, CCamera::AI (2684 B),
  SlicedImage::pack (1580 B), FUN_800502bc, FUN_80059520, FUN_80066bb4, `_._7CCamera`,
  `_._5CRail`. With it on for the whole tree (`GLOBAL_FLAGS=-fstructure-noalias node
  tools/check_all.mjs`) 955 of the 961 matches at the time still matched. The rest were three
  psyq=-pinned files (the flag doesn't exist in those cc1s) plus GetFaceFlags, TrickOb_Update and
  FUN_8004f684, which need it *off*. So it's a per-TU build flag, mostly on. **Try it on any
  near-miss whose residue is a load scheduled below stores, or a global re-read after field
  stores.** Tells: a vptr store landing after a global load in a ctor or dtor; `lw DAT_x` above
  `sh`/`sw` into a prim; a global pointer reused across int/short field stores but reloaded after
  a byte store (byte stores still alias everything).
- **Other compiler versions (`psyq=X` header note, now unused).** The installed cc1s are 4.4 =
  gcc 2.8.1, 4.5 = egcs 1.1.2 (2.91.66, March 1999, the one that matches), and 4.6 = gcc 2.95.2
  (October 1999, after THPS1 shipped). 4.4 and 4.6 hoist those loads by default, which is how the
  `-fstructure-noalias` finding started. `check_all.mjs` still honours `psyq=4.4` (it sets CC1/CC1PL
  and TMPDIR). To sweep drafts with another cc1, set `CC1=D:\Tools\psyqX\CC1PSX.EXE CC1PL=...`.
  The old cc1s (4.0/4.1) only run with `TMPDIR` set to a Windows path; otherwise they fail with
  `\/ctaNNNN: No such file` and leave `cta*` files behind.
- **Loop invariants that are globals:** without `-fstructure-noalias` our cc1 never hoists a global
  load out of a loop that stores to memory. If retail keeps the value in a register through the
  loop, try the flag before reaching for a local. CSmokeJet::Move (240/265 with the flag) still
  copies the pre-loop value into the loop register (`addu t3,v1,zero`) in a way neither form gives.
- **`mflo` then an asm `mtc2`:** ASPSX puts a nop between them. `match_div.py` now treats
  `mtc2`/`ctc2` as reading their first operand, so it adds that nop too.
- **A list walk whose exit test also loads `pNext`:** retail's `beq sN,zero,exit; lw sN,next(sN); j
  body` tail (an unconditional jump back, with `delete` keeping its null check) is
  `for (p = *pp; p ? (pNext = p->mNext, 1) : 0; p = pNext)`, likely a Neversoft macro
  (Ob_QuickAI). Plain `while (p) { pNext = p->mNext; … }` rotates into a bottom test.
- **Case bodies are laid out in source order**, independent of the compare tree. Retail's body order
  0, 2, 3, 1 means `case 1:` was written last (FUN_8005ea74).
- **Old-style g++ vtables:** retail calls virtuals through 8-byte entries (`lh a0,8k(vt); lw v1,8k+4(vt);
  addu a0,this,a0; jalr`), slot k = declaration order with the dtor first. The demo's thunk layout
  (4-byte entries) gives the same slot order at half the offset.
- **`extern RECT X[]` + `r = X[0];`** gives retail's `lui v1; addiu t3,v1,…` before an unaligned
  `lwl/lwr` struct copy (FUN_8006af64).
- **fold reassociates constants within one expression:** `n2 - 2 + b * 2` always becomes
  `n2 + (b*2 - 2)`, and `lo - (hi - 11)` becomes `(lo + 11) - hi`. Retail's literal order comes from
  a temporary in its own statement (`int t = n2 - 2; … t + b * 2`; `int h1 = hi - 11; … lo - h1`).
  This made FUN_8003036c and FUN_80055eb8 exact. Similarly, a product that's halved on two paths
  (one even) is two separate `/ 2` expressions, and cross-jumping shares the final `sra`.
- **MAX/MIN of two byte fields** as `uchar = a < b ? b : a` ternaries gives retail's
  `lbu; lbu; andi; andi; sltu` prologue. `if` forms and `int` results don't (FUN_80055eb8).
- **For big twin functions, iterate with `D=1 OPS=1 python tools/match_div.py …`.** It aligns on opcodes
  and immediates with registers ignored, so only structural hunks print (a load folded into an
  offset, a statement in the wrong place). Fix those first; register allocation usually falls into
  place afterwards. Decomp_GetAnimTransform (1348 B) went from 294 aligned words to exact in three
  edits: `&p[i*2+1]` instead of `(char*)p + i*8 + 4`, splitting `x = base + n*f` into
  `x = base; x += n*f;`, and an init moved into the declaration.
- **Brute-force statement order when only ordering is left.** For independent stores (e.g. three
  velocity components in each of two switch cases), try every permutation of the statements plus
  equivalent comparison forms (`ABS(vx) > ABS(vz)` vs `ABS(vz) < ABS(vx)`). It's a small script
  over a template, and it took CollideWithSkater from 342 to 368/370 in one run. Related findings
  from the same functions:
  - a branch-free `abs` is the `((x)^((x)>>31))-((x)>>31)` macro, not `abs()`;
  - a loop comparison evaluated right operand first (`Best[Order[j+1]] > Best[Order[j]]`);
  - a callee returning a measured width rather than a pointer (VerticalMenu: a `sw` into
    `list[sel].w`).
- **GTE details from the twins:** `gte_sqr0` vs `gte_sqr12` (the sf bit, 0x0a00428 vs 0x0a80428).
  An inline setter with `int _x,_y,_z` params (the SYM shows them as a block's locals) gives
  full-word `lw` loads where plain field copies narrow to `lhu`. Per-register `mtc2`/`ctc2` asm
  statements with C-level loads give retail's `lw t2; ctc2 t2,$n` runs.
- **Copy fields through locals when retail does all the loads before the stores** (FUN_8006c4f0, UVs
  from a texture record into a POLY_FT4). Plain `p->a = q->a; p->b = q->b;` interleaves them, since
  without strict aliasing every store may alias the next load.

**More from the S23 near-miss sweep (1055 → 1079), each verified on retail:**

- **Global-alloc priority is `floor_log2(refs) * refs / live_length`** (`cc1 -dg -dl` prints the
  stats). Most two-register swaps come down to this formula:
  - Reuse an existing variable as a temp to raise its refs (FUN_80043f70, FUN_800609c8).
  - Wrap a counter's `j = 0` and its loop in `do { … } while (0)` so sched1 can't float the init up
    and lengthen its life (FUN_8006edb8).
  - Write a `goto`-shared tail compare in each branch and let cross-jumping merge it (LineToSphere).
- **Spill-slot order = pseudo-creation order.** In C++, declaring a local at its first assignment
  (after a division) numbers it later and gives it the higher stack slot (SlicedImage::rotateDraw).
- **Loop-preheader order:** egcs runs loop.c twice. Pass 1 hoists only the first few invariants, and
  fewer as the body grows; pass 2 hoists the rest. Change the body's size or shape to move a setup
  between constants (`short f` vs `uint f` in EndGameDisplay). Check the "moved to" lines in `-dL`.
- **Wrong arity shows as a stale arg register.** An `a3` (or `a1`) that is only a leftover value or a
  store base before the `jal` means the callee takes fewer args (FUN_800507e4, FUN_800506d0).
- **A call arg rematerialized as `addiu a0,sp,16`:** cse1 swaps a frame address for any register in
  the block holding it. Route every index through `char *p = buf;` and pass `buf` (FUN_8004b0e4).
- **`-fstructure-noalias` only applies to stores through a struct type.** It does nothing for
  `*(int *)(p + off)` with `char *p` (FUN_800506d0).
- **Inline setters with `int` params** (`o->SetM40(100)`, `CVector v(0,0,0)`, `short[3]` globals
  set in reverse address order) give retail's hoisted constants and store order (CSmokeGenerator::Move,
  `_GLOBAL_.I.camDist` at 0x8007d6e4).
- **An unfolded compare of a known zero** comes from a value only RTL proves zero (`unsigned char Life
  = ushortFlags >> 16;`, Exp_HitEnvItem). Don't use an asm barrier.
- **In C++, a `continue` in a `for` never fills its delay slot from the target** (a NOTE hides
  LOOP_VTOP from reorg). If retail has the increment in the slot, use `i = -1; while (++i < N)`
  (WinLoseDisplay).
- **One `mtc2` per asm statement** for hand-rolled GTE loads (`gte_ldVXY0(x); gte_ldVZ0(z);`)
  changes local-alloc around the packing math (RenderShadow).
- **Splitting a constant offset** (`d = base; d += 68;`) changes sched1's first pick (spoolPRE).
- **Table-read spellings:** `(tbl + i)->c` with `extern T tbl[1]` shares one address (QFromXRot).
  `bb = tbl;` assigned after the index puts the `la` between the `sll` and `andi` (FUN_80077408).
- **Global→global copies in both arms, with no locals,** let cross-jumping merge the store tails
  (FUN_80059618).
- **`-fno-gcse` fixes some invariant-load orders** (Card_GetDirAndFreeBlocks 68/69), but it breaks
  other matches in the same TU, so it isn't adopted.

**More from the S24 big-twin pass (1079 → 1082), each verified on retail:**

- **Global-alloc order:**
  - The `;; N regs to allocate:` line in `cc1 -dg` (`.greg`) lists pseudos in priority order.
  - Pass 0 also skips registers that *lower*-priority conflicting pseudos prefer (`;; N preferences:`).
  - Lengthening one GCSE pseudo's life with a temp can therefore move other pseudos' registers. In
    Front_Update, `v = vol*6/5; A = v; B = v;` took the score from 2042 to 2314.
- **`a = b = x` vs two statements** moves the address high/lo_sum in sched1 and changes GCSE
  live lengths. It isn't neutral.
- **A constant store's statement position decides a neighbour's register.** Take the order from the
  demo line table. Moving `m710 = -1;` made the CBruce ctor exact.
- **sched1 loop-note barrier:** the first insn after `NOTE_INSN_LOOP_END` is a barrier, and every
  later reader depends on it. Wrap the stores *before* a call in `do{}while(0)`, not the call
  (ReactToCollision vRight).
- **Pointer bumps:**
  - `uint c = *pw++; p = (ushort *)pw;` keeps retail's unfolded `addiu; lhu 0` bumps.
  - `p = (ushort *)(pw + 1)` lets CSE fold them into offsets.
  - Seen in ExecuteCommandList.
- **Two-arg calls with the bump in the `jal` delay slot:** read the args into locals first:
  `ushort a = p[0], b = p[1]; p += 2; cam->F(a, b);`. The plain form gets cross-jumped with the
  else's `p += 2`.
- **Short field stores:**
  - An `lh` feeding a short field comes from an inline setter with a `short` parameter.
  - An inline 2-arg setter `{ x = a; y = b; }` gives `lhu; lhu; sw; sw` instead of interleaved
    loads and stores.
- **A global address rebuilt before a store in a big loop** (`lui; addiu; sw x,4(sN)`): set
  `T *pG = &G;` inside the loop's entry guard (the preheader). `G.f = x` gets folded because loop.c
  calls the hoist "not desirable" (LineToThisItemCached).
- **Walk an array of structs with `p->f … p++`**, not `p[i].f`. That gives retail's reduced givs
  and frame size (LineToThisItemCached went from 1077 to 1197).
- **Hidden `case N: default:`** shows up as an `eq K; slti K+1; bne → default` tree. Search
  `casetree.py` with the `ge K+1` form (Front_Display).
- **A clamp `f(x <= K ? x : K)`** loads straight into the arg register. `x > K ? K : x` adds a copy.
- **A row counter set mid-argument-setup** comes from `Line++*18+60` post-increments that CSE folds.
- **Statement order can block an unwanted cross-jump** between cases: write
  `SwitchOffPauseMode = 1; EndCode = 7;` in one case so it stops sharing another case's tail.
- **Still unexplained:**
  - HandleJump's lone dead `mult` from an unused `/10` (now matched with inline asm). Flow deletes the whole chain whenever the
    quotient is dead before combine. S29, checked against the egcs 1.1.2 source:
    - Combine's `distribute_notes` deletes the mult along with the rest, since `single_set` is non-null.
    - Reload's `delete_dead_insn` walks a linear chain, so it always leaves the `x>>31` sra.
    - Only a post-reload `delete_computation` gives retail's shape, and it is disabled under
      `-fschedule-insns2` (jump.c:3658).
    - About 75 source shapes tried (cross-jumped ifs, switches, loops, inline helpers, bitfields,
      statics). None leaves a lone mult, and Psy-Q 4.3/4.4/4.6 score far worse.
    - Old Psy-Q 4.0/4.1 cc1 needs `TMP` set or it fails, writing `cta*` files into the cwd.
  - ReactToCollision's unfilled `beq; mult` slot. reorg's `find_dead_or_set_registers` gives up
    after the 2nd conditional branch.

**More from the S25 mid-size sweep (1082 → 1095), each verified on retail:**

- **An argument register you can't explain is often a missing parameter.** ParseScript's opcode sat in `a1`
  because the virtuals take it: `ExecuteCommand(ushort command)`. The same fix filled a loop delay slot.
  The stale-register form of the arity lever applies here too (FUN_8006582c).
- **A wrong struct stride shows up as merged index arithmetic.** If retail computes `(n-1)<<2` and
  `e*stride` separately and yours is one `(e*K+n-1)<<2` chain, recover the stride from the multiply
  sequence (`sll 4; subu; sll 3` = ×120) (FUN_800491e8).
- **Fake `volatile` blocks jump threading.** Typed struct fields give the same reloads and let a `bne`
  thread into the nested test (FUN_8005f9ec).
- **Reassociation:**
  - `ptr + (x + K)` keeps its grouping only when `ptr` is a real pointer.
  - `X - (Y + C)` folds to `(X - C) - Y` even across a variable; use named temps
    `total = a - b; used = S + 16; avail = total - used;` (Mem_NewX).
- **Copies:**
  - A load through the copy's register means the copy is mentioned later. Reuse it in the tail
    (`pLast = pChoiceLast; if (pLast) pLast->pNext = …`, Mem_NewTop).
  - `i = n; …; n = m; i -= 2;` keeps `addu; addiu -2` (Mess_UnloadFont).
- **Quotients:** `t /= 6; f(t);` keeps the quotient in t's register; `f(t / 6)` puts it in the
  argument register (UpdateShadow).
- **Shared constants across blocks:** `X[i] = one = 1;` at the first use, `Y = one;` later (FUN_80066768).
- **The prologue's `sw ra` stays first** with an aggregate initializer, `char *l[2] = {A, B};`. Element
  assignments let sched2 sink it (FUN_80060854).
- **Follow the demo line table:**
  - A statement split over two lines needs its own temp: `int pad = ((w*h)&1)*2; db += w*h*2 + pad;`.
    It changed giv combining elsewhere and was worth 50+ words (Mess_LoadFont).
  - Three tests on one line is one `||` condition.
  - The masks go inside the assignments (`a = *p & 0xdf;`), and cross-jumping merges them (Spool_FindAnim).
- **Byte stores:** `x << 2` into a byte field narrows the load to `lbu`; `x * 4` keeps `lw`.
- **Unrotated top-tested loops** come from `for(;;)` whose exits are `return`. A `break` or a `while`
  condition gets rotated (Mess_UnloadFont).
- **Jump threading:** after `if (a != b) break;`, test `!a` rather than `!b` to steer the zero exit
  (Spool_FindAnim).
- **Operand order inside multi-term `|`** changes sched1 and register choice. Put the constant second
  (LoadTexturesToVRAM).
- **Header syntax:** write `flags=-f…` with no brackets. salvage ignores `[flags=…]`.

**More from the S26 big-function round (1095 → 1101), each verified on retail:**

- **`do { … } while (0)` raises global-alloc priority.** flow adds loop_depth to REG_N_REFS, so refs
  inside the wrapper count double. Wrapping straight-line, macro-like calls fixed Front_Update's
  s0/s1/s2 allocation without changing any code, and it gave DoOnGroundPhysics's `&mKick` the `fp`
  register. Wrapping regions that contain branches changes jump threading and hurts.
- **A block-ending `return` from an inline function, or `goto label;` where the label sits just before
  the next statement,** stops sched1 hoisting a later global load into the block (PlayAway:
  `EndCode = 9; goto done;`).
- **Callee return type shows in loop-entry shape too:** retail's partially cross-jumped loop test came
  from a body call declared `int` (PlayAway, Pad_Update).
- **Spilled shared constant:** when loop.c hoists a shared `1` that global-alloc then spills (it shows
  as `li tN,1` re-created at each use), give one group of uses its own local:
  `int OffRail = 1; … m740 = OffRail;` (DoOnRailPhysics, 927 → 1100).
- **Stack slots:**
  - A named temporary keeps its slot alive: `CVector Diff = a - b; if (Diff.Length() > 30)`.
  - A function-scope C++ local declared after a by-value call takes the call's freed arg slot.
  - Unused separate `CVector` locals pad the frame in 16-byte steps; a `CVector[5]` pads only 60.
  - `CVector v = expr;` as an initializer writes straight into v; an assignment adds a temp.
- **Range tests:** consecutive case labels (`case 62: case 63: case 64:`) give a signed `slti` tree.
  An `if` range folds to `addiu; sltiu`. A hidden empty `case 0: break;` shapes the compare tree.
- **Clamps and arithmetic:**
  - A clamp that stores only when in range: `if (x - d >= -Max) x -= d;`.
  - A variable assigned only positive constants lets `(v >> 12) / 2` become `srl 13`.
  - Per-stat rates go in one nested `?:`.
  - Reuse a variable for an intermediate to pick its register: `B = 4096 - A; B = B * 6144 / 208;`.
  - `-(x << 12) >> 12` survives; `(-x << 12) >> 12` is simplified away.
- **fold distributes `x * (?:)` into the arms only when `x` is static data or an inline call:**
  `int GetBalanceVel() { return mBalanceVel; }` does it; the member itself doesn't (HandleStickToRail).
- **egcs PRE (in gcse) moves a subtraction up to just after its operands' last assignment.** Write
  `X0 = (x0 - xL) / l;` at the division, not an early `X0 = x0 - xL;` (M3dZone_LineToItemNotSCached,
  163 → 530). When a block's registers disagree with the SYM's variables, it uses different variables.
- **Loops:**
  - A redundant second exit stops cse1 carrying loop registers past the loop:
    `while (n < 7) { if (…) break; n++; if (n >= 7) break; }`.
  - A local table pointer keeps two pointer givs apart: `STrick *pTab = T;` for the body, `T[i]` for
    the test.
  - Bounds checks the demo puts on the `while` line belong in its condition.
- **Re-index arrays in every statement** (`img[c][j]`) instead of taking a pointer (board select).
- **Args:** `char *pName = T[i].pName; f(i, pName, T[i].Score);` gives score-first argument loads.
- **Keep `(m & 0x4000) || (m & 0x2000)` as two ifs on a local,** or fold makes `& 0x6000`.
- **Two `= 1` stores in separate arms**, not `b = (expr);`, keep a boolean's `andi; slti` form.
- **Copies between neighbouring records:** records addressed by their own `lui/addiu` are separate
  extern globals; one indexed array shares the base register.
- **Wrong boundaries:** `functions.tsv` can split one function at a `jal`/delay-slot boundary.
  - Tells: no prologue, and no outside `jal` or pointer.
  - Merge the rows; the score uses `size=<total>` in the header meanwhile.
  - DoOnGroundPhysics was 3 rows. It is 5664 B.
- **From the 2P board select (exact from scratch, 9.8 KB):**
  - **Loop bound in a variable:** `int lastBoard = 7;` set before the player loop gives retail's
    never-folded `li t9,1; beq t9,zero` entry guard. A literal `7` lets cse1 fold it away.
  - **gcse hash order sets spill slots:** the slots of hoisted `sp+N` addresses follow gcse's
    hash-bucket order, which shifts with the function's total insn count. Fix length differences
    elsewhere before chasing slot numbers.
  - **Globals:** initialised globals are emitted to `.sdata` before the TU's string literals.
    Zero-init ones that retail puts in `.sbss`/`.lcomm` are `static`. Pin `".sdata"` in syms.json
    when the TU emits literals.
  - **Aligned string copies:** `typedef char T[6] __attribute__((aligned(4))); T s = "board";` gives
    `lw/lh` copies instead of `lwl/lwr`.
  - **Split stores:** `x[0] = v; x[1] = v;` controls both the store order and the address-pseudo order.
  - **Per-loop counters:** `for (int p…)` in each loop keeps one function-wide spilled counter out.
  - **Loop shape:** `while (!result) { … break; }` is top-entered; `do … while` duplicates the head.
- **Still open:**
  - Front_Display: a dead `s2 = 0` that must be live at flow; a 2-insn partial cross-jump that needs
    the two branches to differ before jump2.
  - DoOnGroundPhysics: an `s1`/`s2` pass-0 preference.

**More from the S27 sweep (1103 → 1172 functions), each verified on retail:**

- **Allocation priority:**
  - Nested `do { do { … } while (0); } while (0);` scales refs non-linearly through `floor_log2`.
    local-alloc ties a 3-address op's output to its dying first input, so one quantity can carry
    ~4× a neighbour's refs. Two wrappers flip a tie that one can't (boardselect1p).
  - A wrapper's loop note is a sched barrier, so where it starts matters.
  - `do{}while(0)` around error-return regions turns them into loop exits; reorg then fills those
    delay slots from the fall-through (Card_Load).
- **Struct layout:**
  - A true struct layout keeps the displacement in the load. Modelling the board record as
    `Boards[]` at +0x54 with `Req` at +8 stopped a hoisted `&G+92` invariant (GetStats).
  - A wrong stride shows as merged index arithmetic.
- **Scheduling:**
  - sched1's tie rule: a register-setting insn whose pseudo is set once and stays live gets top rank
    (`7f000001` in `-dS`); stores keep their critical-path priority. A variable set twice moves
    within its block.
  - Argument moves land low only when picked to fill a load-delay stall.
- **jump.c rewrites `if (c) x = A; else x = B;` into a conditional set.** Start each arm with
  another statement to keep the `bne; j` (DisplayTrick).
- **Addressing:**
  - A block-local pointer set once keeps alias.c's base, so a load through it can move above a byte
    store to another global (controllerhandler).
  - Column-first addressing, `int *b = &T[0].f[sel]; int *f = b + row*4;`, emits the table `lui`
    before the row shift.
- **Copies and folding:**
  - A shared `goto out; … out: return ret;` stops a returned zero being folded to a constant.
  - A zero-offset load through an `int` temp removes the base→value register preference (egcs
    `set_preference`; ParsePSX went from 220 to 336).
  - g++'s cast-as-lvalue `*((short *)p)++` keeps unfolded pointer copies (SetZone).
  - `0xff00U` keeps a short-store mask unsigned (`andi`, not `li -256`).
  - `int Frames = 5; x / Frames` keeps a real `div` by a register.
- **Builtin string functions:** declare `extern "C" char *strcpy(char*, const char*);` (also strcmp,
  sprintf) and map it, so constant copies expand inline as `lw/lb` moves. Literals of 8 bytes or less
  go to `.sdata` after the TU's initialised globals.
- **Loop shape:**
  - A main loop with only `goto done;` exits (no `break`) isn't rotated by `expand_end_loop`.
  - A plain `goto` loop loses its loop notes, and with them the loop-depth ref weighting.
- **Cross-jumping:**
  - Per-case calls get cross-jumped into a shared `jal`.
  - Writing cross-jumped tails out in full raises refs without changing code.
- **Callee identity:**
  - An untouched `a0` at a `jal` can still be `f(param)`; the extra use raises the param's priority.
  - FUN_8008b900 is `sprintf` and returns `int`. Wrong `void` return types caused several of the
    remaining residues.
- **End-game levers (the last exact matches):**
  - **sched1's set-once boost:** an insn setting a pseudo that is set exactly once and still live
    gets top priority and lands right above its consumer. Assign the variable in two places (both
    if/else arms) so it can stay early (FUN_8003159c, HandleStickToRail). Index with `[i - 2]`
    rather than `i -= 2` to glue it to its use (Mess_UnloadFont).
  - **Removing wrappers:** a `do{}while(0)` from an earlier pass can *cause* a residue (its loop note
    is a sched barrier). Recompile with every wrapper removed (TrickTips_Update).
  - **Goto loops:** an outermost `label: … goto label;` loop has no loop notes, so refs aren't
    loop-weighted and short-lived constants stop winning v0 (front-end dispatcher).
  - **Priority ties:** an equal global-alloc priority goes to the lower pseudo number, i.e. the
    earlier declaration. A duplicated, later cross-jumped tail can raise refs in place of a wrapper.
  - **Expression chains:** block-local temps for an or-chain's low-priority terms decide
    local-alloc's chain register (M3d_PreprocessWibblyTextures).
  - **Check `;; N conflicts:` in `.greg`** before trying priority levers. A hard register listed
    there means local-alloc already took it, and no priority change will help.
  - **A "separate" global may be a struct member:** check whether an unexplained `DAT_` sits just
    past a known struct's fields. FUN_8002566c's `DAT_800a706c` is `DAT_800a6844 + 0x828`, i.e.
    `DAT_800a6844.b828[j]`. Spelling it as a member changed the loop pass's hoist order
    (sp+16 before sp+48) and made the 1-word residue exact. The bytes are identical either way.
- **Parked, with mechanisms understood but no plausible source found:**
  - HandleJump: EXACT in S29 via one inline-asm `mult` (end-game policy, parked for good). Plain C
    cannot leave a dead `mult` without `mfhi` under Psy-Q 4.5.
  - ReactToCollision: retail's reorg follows 2 conditional branches to our 1.
  - DoOnGroundPhysics: needs an invisible s1 preference.
  - Bit_Display: local-alloc qty order.
  - HandleStickToRail, FUN_8004f000, Card_GetDirAndFreeBlocks, FileIO_Open.
- **Function table:**
  - Identities: FUN_8003058c is HandleStickToRail; FUN_80059794 is the 1P skater select;
    FUN_8006658c/80066d7c are the level select.
  - The `Mem_Delete__FPv` row at 0x8006e19c is misnamed: it wraps CPanelData::Trick.

## Global-referencing functions — UNLOCKED (linker-lite reloc resolution)

**THPS1 is `-G8`** (not -G0): small globals are addressed **gp-relative**, `gp = 0x800d1080`
(derived from `sb a0,1392(gp)` → `0x800d15f0`). Functions with no globals match with `-G0`/`-G8`
either way; global-referencing functions need the gp layout.

`tools/match_sym.mjs <src.c> <addr> <size> <syms.json> [-O2]` handles them: reference a global by
name in the C (**define** it, e.g. `unsigned DAT_800d15f4;`, so cc1 emits a **GPREL16**/HI16/LO16
relocation), give its address in `syms.json` (`{"gp":"0x800d1080","DAT_800d15f4":"0x800d15f4",…}`),
and the tool resolves the relocations (gprel = `sym-gp`, hi/lo = split addr) before byte-diffing —
no full linker needed. Verified: `sb a0,1392(gp)` resolves exactly. Global addresses come from the
retail Ghidra `DAT_########` labels.

`match_sym.mjs` resolves **GPREL16** (small globals), **HI16/LO16** (large data), and **MIPS26**
(function calls *and* local control-flow jumps — the latter via the function's own base address).
Symbol addresses come from retail Ghidra `DAT_########` / `FUN_########` labels.

## Byte-exact functions: `matched.tsv` is the source of truth (820 as of 2026-10-04)

The first 32, by hand:

- **VECTOR / CVector (19):** all 14 operators above + `operator/=`, `operator/(V,int)` (via
  `match_div.py`), `KillSmall`, `Length` (GTE, see below), and `Normalize` (div + call).
- **CSVector (4):** `operator+=`, `operator/=` (div), `Mask`, `KillSmall`.
- **PAD (4):** `Pad_ClearTriggers`, `Pad_LockAnalog`, `Pad_ClearAll`, `Pad_Clear`.
- **M3DUTILS (2):** `M3dUtils_AnglesFromMatrix` (172B, 4 calls incl. `ratan2` — 43/43 first try),
  `M3dUtils_NormaliseVector` (304B: GTE + 3 divs + 2 calls + loop, 76/76 first draft).
- **MESS/SFX/UTILS (3):** `FUN_8001d250` setter, `Mess_SetRGB`, `SFX_Stop`.

Observation: **structured functions with calls/control-flow tend to match on the first try** (call
sequence constrains ordering) — `AnglesFromMatrix` did. The fiddly cases are small register-shuffle
functions (regalloc freedom) and struct-array field offsets (`ReadHooksPacket` needs
`struct S array[]; array[i].field`).

## GTE — UNLOCKED (Psy-Q SDK headers + DMPSX fixup)
GTE code uses Psy-Q's `gte_*` **inline macros** (`INLINE_C.H`). Write the C exactly as Neversoft did:

    #include <sys/types.h>
    #include <libgte.h>
    #include <inline_c.h>
    ... gte_ldlvl(&t); gte_sqr0(); gte_stlvnl(&t); ...

- **Headers** (copyrighted, outside the repo): `D:\Tools\psyq4.5\include\` — the `INCLUDE\` tree from
  the full 4.5 SDK, `archive.org/download/ps1_sdks/PSYQ.SDevTC.Developers.Toolkit.For.PSX.v4.5-MFD.zip`
  (a zip of 7 zips holding a multi-volume RAR; `7z x mfdpsx.rar "INCLUDE\*" -r`). Override the path
  with `PSYQ_INC`.
- **Preprocessing:** cc1 has no cpp (it can't even strip a `/* comment */`), so every matcher
  (`tools/psyq.mjs` → `matchcc`/`match_sym`/`permute`; `match_div.py`) runs **`CPPPSX.EXE`** first,
  as CCPSX does. Comments are fine in sources now.
- **DMPSX:** `INLINE_C.H` emits loads/stores (`lwc2`/`swc2`) directly, but each GTE *command* is a
  placeholder `.word (n<<6)|0x3f` that Sony's **`DMPSX.EXE`** patches into the real `cop2` word
  post-assembly (and it insists on the macro's 2 leading nops). `match_div.py` runs the real
  DMPSX on its ASPSX object; the psyq-asm matchers apply `tools/dmpsx_table.json` (all 65 commands),
  which `tools/gen_dmpsx_table.py` derives by running DMPSX. E.g. `gte_sqr0` `0x00000f3f` → `0x4aa00428`.
- Retail uses the **`INLINE_C.H`** form (the base register is the argument register), not
  `INLINE_O.H` (which routes through `$12`).

Verified: **`CVector::Length` (0x8004d900) 32/32** (`src/vector/length.c`, `match_sym.mjs`) and
**`M3dUtils_NormaliseVector` (0x80081f24) 76/76** (`src/m3dutils/normalisevector.c`, `match_div.py`
since it also divides). Both matched on the first draft. Next: the M3D/physics GTE users.

`Normalize` (0x8004d980) is **not** a GTE function: it's `jal Length` + bare `div`s, 24/24 via
`match_div.py`. Retail never reloads `z`; it stores `(y/len)/len` there, which looks like a bug in the
original source (`v->z = v->y / l`). It's kept as-is for byte-exactness.

PAD-module tip: trailing globals (e.g. `DAT_800c0b90..93`) are addressed as offsets into the array
base `DAT_800c0a48` (`lui 0x800c; addiu 0xa48; sb 0x148(v0)`), so reference them as
`((char*)DAT_800c0a48)[0x148]` to match.

Close (near-miss, permuter/scheduling): `Rnd` 20/23, `AddEntry` 30/34 (one commutative operand-order
pattern), `ChoiceIs`, `Mess_TextHeight`. Deferred (hard): CSVector free operators (unaligned
small-struct returns).

Detail of the non-VECTOR exact matches:

| function | module | calls | words |
|---|---|---:|---|
| `FUN_8001d250` (global setter) | MESS | 76 | 3/3 ✅ |
| `Pad_ClearTriggers` (loop) | PAD | 86 | 8/8 ✅ |
| `Mess_SetRGB` (global write) | MESS | 89 | 15/15 ✅ |
| `SFX_Stop` (5-global RMW) | SFX | 25 | 18/18 ✅ |

**Close but needs a permuter** (logic correct; register-allocation/scheduling differs): `Rnd__Fi`
20/23, `ChoiceIs` 7/16, `Mess_TextHeight` 2/19. Pure functions (pointer arith / global RMW) match on
the first/second try; functions with register-allocation freedom or specific scheduling need
permutation.

## Permuter — `tools/permute.mjs` (wired)

An integrated permuter over the same compile+reloc-resolve+score pipeline: it mutates the C
(commutative operand swaps, temp introduction on `x&mask`/`x>>n`/paren subexprs, adjacent
independent-statement reorder) and searches with **simulated annealing + compound mutations** for a
byte-exact variant.

    node tools/permute.mjs <src.c> <addrHex> <size> <syms.json> -O2 --iters N

Mutations: commutative operand swap, temp introduction, adjacent independent **statement *and* temp-
declaration** reorder, and a **cleanup pass** (collapse alias temps, drop unused temps). Search is
simulated annealing with compound mutations; new bests are auto-cleaned.

**Validated to EXACT:** from a 9/15 `Mess_SetRGB` it reached **15/15 byte-exact in 56 iterations**,
autonomously (extract masks → reorder the temp decls to argument order → fix operand order → clean up
redundant temps). So it closes **structural** mismatches (eval order, temporaries, operand/decl
order) to exact on its own.

Boundary: **deep instruction-scheduling** cases it can't express — e.g. `Rnd`'s delayed store (its
three RNG statements are mutually dependent, so no operand-swap/temp/reorder changes GCC's store
placement). Those still need the full AST-based `decomp-permuter-psyq` or hand-tuning.

## Compiler — CONFIRMED: PsyQ 4.5, `-O2 -G0` (globals: `-G8`, gp=0x800d1080)

Found by sweeping every Psy-Q build against `CVector::operator+=`: **`D:\Tools\psyq4.5\CC1PSX.EXE`
(GCC 2.8.1) with `-O2 -G0` reproduces the retail bytes exactly** (THPS1's PSX-EXE has `gp=0` ⇒ -G0).
psyq4.0/4.1 (2.7.2), 4.3/4.4 (other 2.8.1) and 4.6 (2.95.2) all miss — the exact SN build matters.
**Correction (2026-10-04):** psyq4.5's `CC1PSX -version` banner says **egcs-2.91.66 (egcs 1.1.2)**,
not 2.8.1. 4.3 is 2.8.0 and 4.4 is 2.8.1. Re-checked on CCamera::AI: 4.5 gets 670/671 aligned, 4.4 gets
489 (it uses t0-t2 for block copies where retail uses t1-t3) and 4.6 gets 446. Set `CC1=… CC1PL=…` to
try another build in either matcher; `NOEXC=1` drops `-fno-exceptions` for front ends that lack it.
All compilers run **natively on Windows** (original SN `*.EXE`s, no wibo); `D:\Tools\psyq4.5` also has
`ASPSX.EXE`/`CCPSX.EXE`/`CC1PLPSX.EXE`.

**Canonical matcher:** `node tools/matchcc.mjs <src.c> <retailAddrHex> <size> -O2 -G0` (defaults to
psyq4.5) = compile → `psyq-asm` → byte-diff vs `SLUS_008.60`, printing `N/N >>> EXACT <<<` on a match.

## Byte-exact so far — 14 / 28 VECTOR functions

| function | retail | words |
|---|---|---|
| `CVector::operator+=` | 0x8004da3c | 15/15 ✅ |
| `CVector::operator-=` | 0x8004da78 | 15/15 ✅ |
| `CVector::operator*=(int)` | 0x8004dab4 | 20/20 ✅ |
| `CVector::operator>>=(int)` | 0x8004db54 | 15/15 ✅ |
| `CVector::operator<<=(int)` | 0x8004db90 | 15/15 ✅ |
| `CVector::operator==` | 0x8004dc18 | 17/17 ✅ |
| `CVector::operator!=` | 0x8004dc5c | 18/18 ✅ |
| `CVector::operator-` (unary) | 0x8004df2c | 11/11 ✅ |
| `operator+(CVector,CVector)` | 0x8004dca4 | 22/22 ✅ |
| `operator-(CVector,CVector)` | 0x8004dcfc | 22/22 ✅ |
| `operator*(CVector,int)` | 0x8004dd54 | 26/26 ✅ |
| `operator*(int,CVector)` | 0x8004ddbc | 26/26 ✅ |
| `operator>>(CVector,int)` | 0x8004de8c | 20/20 ✅ |
| `operator<<(CVector,int)` | 0x8004dedc | 20/20 ✅ |

C source in `src/vector/*.c`. Each verified with `matchcc.mjs <c> <addr> <size> -O2 -G0`.

### C-writing patterns learned (essential for matching)
- **Member op returning `CVector&`** (`+=`,`-=`,…): write `V* op(V* this, …){ …; return this; }`
  (the `return *this` produces the leading `move v0,a0`).
- **Free op returning `CVector` by value** (`operator+`, `operator*`, …): write it to return the
  struct **by value** (`V op(…){ V r; r.x=…; return r; }`) — the compiler builds `r` on the stack
  then copies to the hidden return pointer, exactly as retail does. Writing through an explicit
  result pointer does **not** match.
- **Independent loads:** load fields into locals first (`int x=-a->x,y=-a->y,z=-a->z;`) so the
  scheduler hoists them like retail (avoids store-vs-load aliasing conservatism).

### Patterns from the grind (agent-discovered, each verified on a real match)
- **`v0` vs `v1`/`a2` only:** declare the callee with a non-void return type and ignore the result
  (`int f(void);`, `V* __apl__…`). That reserves `v0` around the call. Try this first.
- **Branch layout:** swap `if (x) return A; return B;` ↔ `if (!x) return B; return A;`. Retail
  often puts the error block first and success last. Use per-branch `return expr>>12;` to stop
  cross-jumping. `if(c)f(a);else f(b);` gives a shared call tail; `?:` doesn't.
- **Rotated list walks** (retail preloads `next`, loops back with `j`):
  `if(!m)return; n=m->next; for(;;){ …; m=n; if(n) n=n->next; else break; }`
- **Switch with a shared tail:** `case 0: …; goto tail; case 1: …; tail: …`.
- **Statement order drives both register assignment and store order.** For about 7 independent
  statements, brute-forcing the permutations works where `permute.mjs` stalls.
- **Arrays:** repeat `DAT[idx].x, DAT[idx].y` instead of taking a `&DAT[idx]` local. Use a
  `short* row = tab[t];` temp for 2-D lookups. Explicit array fields (`h->title[i]`) stop gcc
  strength-reducing loops the way retail didn't. `data[i*w+j].a` matches where
  `data+(idx<<2)` swaps `addu` operands.
- **Arithmetic:** `mult` operand order follows source order. `short s; s = s + (p[i]+1);` gives
  the `andi/sll/sra` sequence, and an explicit `& 0xffff` does not. `/320` by constant is a
  reciprocal `mult`, so `match_sym` handles it with no div.
- **Globals:** use `char X[8];` for gp-relative globals whose *address* is taken (`addiu a0,gp,N`).
  Use `__attribute__((section(".data")))` to force `lui` addressing of a small global. Six-byte
  struct globals copy with `lwl/lwr`. Separate `DAT->field` statements reproduce retail's
  aliasing reloads after `char` stores.
- **Misc:** a never-returning function ends with `for(;;);` (`jal` + self-jump). cc1 is **C89**:
  a declaration after a statement fails with a bare exit 33. If only a `jal` word differs, the
  `syms.json` address is wrong (`target = (w&0x3ffffff)*4 | 0x80000000`).
- **Flip a commutative `addu`'s operand order:** put the intermediate pointer in a named temp:
  `short *s=(short*)(q+o); short *t=s+b; *t=0;`. `s[b]`, `*(s+b)` and `s+=b` all failed.
- **Table base before an index shift** (`lui/addiu` ahead of `sll`): declare it uninitialised and
  assign it after the masking: `SC *b; a&=4095; b=DAT; t=b+a;`.
- **`(size<<4)|flags` words:** declare bitfields (`unsigned flags:4; unsigned len:28;`) and write
  `p->len += …`. The bitfield read-modify-write gives retail's shape directly.
- **Aligned advance:** `len=(sz+3)&~3; buf+=len;` with a temp changes the scheduler tie-break.
- **C++ front end experiment:** `CC1=D:/Tools/psyq4.5/CC1PLPSX.EXE` with the opt string
  `"-O2 -fno-exceptions"` runs through `match_sym.mjs` unchanged. Constructors returning `this` may
  need it (their `move v0,s0` placement differs from C).
- **Same-register `lui rX; addiu rX,rX` (or `lw rX,%lo(rX)`) for a table or global** means the
  declaration was `extern` with a size of **8 bytes or less** (`extern short SinTab[4];`, `extern int G;`).
  cc1 then can't rule out small data, so it emits the `la`/`lw sym` macro and ASPSX expands it
  with one register. Unsized `extern T X[]`, or a real definition, gives cc1's split `lui v0; addiu rX,v0`.
  This fixed `Move__11CWobblyGlow`. The address range doesn't decide it (that sine table is inside
  the gp window); how retail addresses it does. If retail uses `N(gp)`, *define* the global instead.
- **Local pointer to a global struct** (`SComp *c = &DAT_x; c->f…`) stops gcc strength-reducing
  and hoisting in loops (LadderLogic went from 3/95 to exact).
- **Stack locals are laid out in declaration order** (ascending offsets). One temp reused across two
  blocks must be declared once at function scope, or gcc gives it two slots.
- **Stack-passed parameter used several times:** gcc copies it to an s-reg at entry. Reading it
  once late (`short sa = a;` after the calls) reproduces retail's late `lw`.
- **Loops:** `while (!f(x)) {…}` gives jump-to-test-at-bottom (retail's `j L_test`).
  `x/2` on a signed int is `srl/addu/sra`, with no div. For branch polarity, try
  `dx > 0 ? -K : K` vs `dx <= 0 ? K : -K`.
- **Magic divisors** (`mult` + `mfhi` + shift): 0x10624dd3 with `sra 7` = /2000 (`sra 6` = /1000),
  0x2aaaaaab = /6, 0x2e8ba2e9 = /11, 0xfc0fc0fd with `srl 7` = /130.
- **Neversoft's own GTE code** (Medal_Create, Tape_Create) isn't INLINE_C. It's
  `lw; nop; ctc2`/`mtc2` per word, a bare `cop2 0x406012` (rtv0, sf=0) and `swc2 $25..27`. Use inline asm:
  `__asm__ volatile("ctc2 %0, $" #n : : "r"(*(int*)(cam+off)))`, then
  `__asm__ volatile("nop; nop; cop2 0x406012")`, then the `swc2` block with a `"memory"` clobber.
- **Permuter:** `SHOW=1` writes the best variant even when it isn't exact (it took CGlassBit
  from 107 to 136 of 140). It prepends `_tNNN` temps and rewrites the header, so restore line 1 when
  adopting a variant.
- **Split `lui v1; lw v0,off(v1)` (different registers) for a small global:** use `extern int X[];` and
  `X[0]`. (`extern int X;` gives same-register `lui v0; lw v0`.) For an address argument,
  `f(p,&DAT)` with `extern int DAT;` gives retail's `lui a1; addiu a1; jal; move a0` ordering.
- **Destructors:** `*(char**)(p+68)=vtbl; Base(p,0); if(f&1) operator_delete(p);`. Declare the base
  dtor `int` or `void` depending on whether `andi` lands in v0 or v1. A virtual delete is
  `if(o) o->vt->fn((char*)o+o->vt->off,3)`.
- **Frame padding:** a 16-byte frame with no locals or saves needs `volatile int pad[4];`. If the frame
  is 8 bytes short, add `volatile int pad[2];`.
- **Struct copies:** 6-byte → `struct S6 G[1]; G[0]=*p;` (`lwl/lwr`). 12/20-byte `S t=*src;` → loads, then stores.
- **Index arithmetic:** `(struct E*)(i*36+(int)p->e)` gives `addu a0,base` order where `&p->e[i]` doesn't.
- **`<libgpu.h>`** needs `<sys/types.h>` and `<libgte.h>` included first (otherwise cc1 fails with a bare exit 33).
- **Hard for now:** the 64-bit fixed-point MAC helpers (0x80072038–0x800720ec, likely hand-written)
  and Psy-Q packet builders (`addPrim`/`setPolyF4`: logic is right, register naming differs).
- **A load hoisted into a branch delay slot where retail has `nop`:** make the global `volatile`.
  `extern const int G;` does the opposite and lets gcc hoist the load out of a loop that stores.
- **Inline struct "constructors"** (`static inline void VI(V*p,int x,int y,int z){…}`) reproduce retail's
  "x through an sp offset, y/z through a pointer register" store pattern.
- **Many parallel induction pointers** (13 in `ReadControllers`-style loops): index a flat byte array,
  `B[i*364+0x11] || B[i*364+0x21] …`. Struct-array syntax shares one pointer.
- **`addu` tied to the destination:** `dv<<=1; dv+=t;` gives `addu dv,dv,t`. Writing a fresh variable,
  `dw=(unsigned)t+dv;`, gives retail's operand order.
- **Delay-slot ops computed inside the guarded block** (`if(a){u=t&255;…}`), not before it.
- **Loop-back delay slot `move`:** reset `q=p;` at the top of the loop body, not before the loop.
- **Jump-table case base:** `addiu v1,a1,-8448` means the cases start at 8448. Several jump tables in
  one function are fine: each is placed from its own `lui/addiu` pair.
- **Compare-tree switches** (no table): gcc picks the root by cost (ranges count double), so the
  case set, including empty cases, must match exactly to get the same tree.
- **Aggregate locals are emitted where they're declared:** put a 6-byte `lwl/lwr` struct copy in a nested
  `{}` after the earlier calls when retail does it late. Disjoint scopes share a stack slot.
- **Loop keyword matters:** `while(1)`+`break`, `for(;;)` and `do/while` rotate differently. `for(;;)`
  can add a `j` into the middle of the loop.
- **Stack `V` (12-byte) locals sit 16 bytes apart:** add an `int pad;` or use an `aligned(8)` typedef.
- **Struct-returning operators in C:** `V r=__mi__FRC7CVectorT0(&a,&b);`. For a 6-byte CSVector
  result, declare the callee as returning the 12-byte V and copy `*(S6*)&t`. Reusing one V temp
  for both kinds of result fixes the frame size.
- **By-value struct params** (`V a` in a1..a3) need field-wise stores (`d.x=a.x;…`), not `d=a`. A param
  reloaded from its stack home after calls means it was a by-value struct.
- **Aliasing decides re-reads:** a global is re-read after raw `*(short*)(p+o)=` stores, but stays cached
  across typed struct-member stores.
- **`x*340` indexing:** `(char*)D+x*340` gives retail's `sll/addu` chain. `D[x]` with a 340-byte struct
  gives a different multiply.
- **Short/char params retail doesn't sign-extend:** declare them `int` and cast at the point of use.
- **A counter held as both `i` and `i+1`:** two variables, with `j=i+1;` first in the loop body.
- **m2c drafts:** extra call args are often stale registers (check the callee's real signature),
  array strides can be misread, and `SYM + 0xNNNN` artifacts are just the base symbol. Jump tables
  are supported (the bridge emits them as `jtbl_` `.rodata`).
- **Reuse a parameter as the working variable** (`if(a3<0) a3=-a3; else a3=F(p,a1,a3);`) rather than a new local.
- **An argument register passed through unchanged** means the callee takes fewer args than m2c says.
  Declare it `int F(void)` and so on.
- **`nop` in the delay slot before an epilogue jump:** declare the function `int` with no `return`, so gcc
  won't move a `v0` write into the slot.
- **Shared call tails (`goto`/cross-jumps in m2c):** write each call separately (`if(x) F(a,0); else F(b,0);`)
  and gcc merges the tails itself. Store-then-reload of a global: store in each branch and let gcc merge.
- **One hoisted constant reused** (a1 for two stores): a named local assigned once (`one=1;`).
- **Loop guards:** `while(s0&&s0->f!=v){…}` gives the `beq` guard plus a bottom `bne`. A `for` loop gets
  invariant `lui`s hoisted into s-regs and a `goto` loop doesn't.
- **Two shorts 4 bytes apart on the stack:** `short ab[4]`, used as `&ab[0]` and `&ab[2]`. Unused locals vanish
  and shift the layout.
- **`addu` order via staged pointers:** `q=(int*)(a+K); q=(int*)((char*)q+o);` gives `addiu s0,s3,K; addu s0,s0,s1`.
- **A permuter result that reorders calls is wrong**, even when it scores higher. Review before adopting.
- **Retail recomputes `base + i*K` every iteration (no running pointer):** *if the counter has an
  `andi 0xff/0xffff` wrap*, it's an **unsigned narrow type** (`unsigned char`/`unsigned short i`), which stops GCC 2.8
  from strength-reducing it (verified with cc1). With a plain `int` counter (`slti`, no `andi`) that's
  **not** the cause. Several such loops (0x80048ed8, 0x80041ce0, 0x8007171c, 0x800103a4, 0x80010000) mix
  one running pointer, one offset giv and one per-iteration `sll` chain, and no source form has
  reproduced that yet. Struct-array indexing (`E[i]`, `E[1-i]`) with an int `i` did fix 0x80048d24 and 0x80065fec.
- **Sibling loop nests need their own counters** (`j2`/`k2`). Reusing `j`/`k` changes their registers.
- **`&T[i]` written in the loop body** (no pre-declared `p=T`) keeps the `lui` after the loop guard.
  `ptr=&T[x]; f(ptr-1)` keeps the `-4` as a separate `addiu`, whereas `&T[x-1]` folds it.
- **`sh` before `sll/sra`:** `o->s16 = -o->i32; o->i32 = o->s16;` (no `short` temp).
- **Nested `if(s>=20) if(s<25)`** gives retail's `slti/bne; slti/beq`. `a&&b` becomes one `sltiu` range check.
- **Tail-call arg in the delay slot:** `if(!s4)f(2);else f(0);`. `f(s4?0:2)` becomes `sltiu`+`sll`.
- **`for(;;){…; if(v==-1){a=0;continue;} r=v; break;} return r;`** puts `move v0,v1` last in the epilogue.
- **Constant args to inline helpers get folded:** use plain locals (`int h=1,f=1;`) to keep them in registers.
- **Statement-order search pays off:** try all 24 orderings of 4 independent blocks.
- **A variable assigned in two basic blocks becomes a global pseudo**, and global-alloc then picks
  different registers. Use a distinct variable per block (`t1/t2` → `t1/t2` + `u1/u2`). One `a=__nw()` reused
  at 7 call sites became `a1..a7`. This is the biggest lever on large functions.
- **Word-copy loops** (16-byte `lw/sw` blocks) need an int-typed struct (`struct{int c[34];}`). A char
  struct gives `lwl/lwr` plus alignment checks.
- **Typed table index:** `((short*)T)[((x*600)&0x3ff8)>>1]` gives retail's `sll, andi, addu base` with the
  base `lui` first.
- **Manual /4096:** `s0=prod; if(s0<0)s0+=4095; s0>>=12;`. `x*10/12` uses magic 0x2aaaaaab with `sra 1`.
- **Argument evaluation order:** `f(DAT, cond?X:Y)` loads `DAT` before the condition, as retail does.
- **Explicit temps fix `li` order:** `c=386; a=30; G=c; b=-160;`.
- **Prologue order of `move this` vs a stack-arg load**, and the s-reg priority of spilled pseudos versus
  `fp`, are the most common residual diffs on big functions. Nothing so far fixes them reliably.
- **Block-scope vs function-scope declarations** change register allocation for multi-def variables.
  Try every subset of {function-level, block-level}. `Split` went from 435/493 to 492/493 to exact that way.
- **Hidden-return operators** (`__mi__`, `__ml__`, `__pl__`, …) can be declared `void f(T* ret, …)` with the
  result pointer passed explicitly. `*p=f()` makes gcc build a temp and grows the frame.
- **Force a stack slot without `volatile`:** `struct{int sv;int pad;Q r;} z;`. `volatile` blocks delay-slot fill,
  and a 1-element array gets promoted to a register.
- **`x = e + x` comes out as `addu x,x,e`.** Write `n = e + x; x = n;` to get `(e, x)`.
- **`switch` vs if-chain:** `switch(x){case 0:…;break;case 1:…;break;}` with the shared tail *after* the
  switch gives retail's `beq/beq/j` layout. The `if/||` form doesn't.
- **Table search:** `for(i=0;i<6;i++) if(T[i]>=b) break;` gives the non-rotated top-exit loop.
- **Statement-order hill climbing must be dependency-safe:** never move statements across blocks, calls
  or aliasing memory ops. Unsafe climbers raise the score with semantically wrong code.
- **GTE:** `gte_rtv0tr` (0x4a480012) is not `gte_rtv0`. For a cross product use
  `gte_ldopv1/ldopv2/op12/stlvnl`. For a matrix multiply through a `VECTOR` local use `gte_ldlv0`.
  `cop2 0x170000c` is `gte_op0` (sf=0, the code shifts by hand); `0x178000c` is `gte_op12`.
- **C++ locals declared mid-block reuse freed stack slots.** cc1plus allocates a local's slot when its
  declaration is expanded. Declared after an inner block or a statement with temporaries, it takes the
  slots those released. That is how the SYM shows `U` and `old_pos` sharing `sp+24`, or a CQuat temp and
  `offset_inc` sharing `sp+88`. Declare each local where retail's frame says it starts
  (CM_CompetitionIntro, MoveToDesiredPos).
- **fold() moves a product's constant onto the *other* factor:** `t * (b * 8000)` becomes
  `(t*8000)*b` (shifts on `t`), while `t * 8000 * b` shifts `b` and emits `mult t, b8000`. Pick the
  spelling whose shifted register matches retail. With `t = 49` known, only the shifted factor folds.
- **Template members emit their vtable and inline dtor** (`_vt.t9AnimParam1Z5CQuat`, `_._t9…`) even
  under `#pragma interface`. Map the dtor to its retail address in `syms.json` and the `.rdata` check
  passes.
- **Bump a pointer in place when retail adds the offset in its own register:** `p = f(); p += 6;
  if (*p == 'H')` keeps `addiu a0,v0,6; lbu 0(a0)`. Written `p = f() + 6`, CSE keeps the call result
  live and folds the offset into the load (`lbu 6(v0)`). The same goes for `pTable += 3` after
  reading a header field through the base.
- **Sized vs unsized `extern` arrays change scheduling.** `extern T X[];` makes cc1 emit a split
  `lui`/`addiu` pair (two insns it schedules separately). `extern T X[64];` (or a scalar) makes it one
  `la` macro insn, which reorg won't put in a delay slot and the scheduler orders differently.
  This fixed an index-before-base order (`sll; lui; addiu; addu`) twice: CalculateSpatialAttenuation
  and LineToThisItem. `build/t/externsweep.py` tries the toggle on every near-miss draft.
- **One variable vs two:** declaring a pair once in the enclosing block, rather than separately in
  a loop and after it, makes it one pseudo that lives across a call. It then gets a callee-saved
  register and shifts every other register by one (Pack_Pack's AlignedW).
- **`if (c) x = &A; else x = &B;` stays an if/else** only when each address is a split
  `lui`/`addiu`. An unsized extern array (`extern T G[]; x = &G[1]`) gives that. A sized global
  gives one `la` insn, and jump.c rewrites it to `x = &B; if (c) x = &A;`.
- **Callee return types change register choice** even when the result is unused (`int
  Mess_DrawPanelText` put a constant arg in v1 rather than v0). Try `int` before `void`.
- **A struct member read through a reference** (`SMike &m = G; CSVector &a = m.mAngles;`) gives
  retail's `addiu v1,G; addiu a1,v1,24` base pair with mixed `n(a1)` and `24+n(v1)` loads.
- **`base[idx]`, not `p = base; p += idx`.** The pointer-bump form emits `addu rX,idx,base`, but
  retail usually has `addu rX,base,idx`. This fixed FUN_8005cba0 and FUN_8005d00c.
- **Switch cases: `break` vs `return 1` decides which tail cross-jumping keeps.** Two cases ending
  in `pScript = p; MoveTo(pos);` merged in the wrong direction until they ended in `break` (falling
  to the function's `return 1`). Seen in CPlatform::ExecuteCommand.
- **Follow the demo's per-line store grouping over a libgpu macro.** Mick_Darken's quad corners match
  only as `x0,x1 / y0,y1 / x2,x3 / y2,y3` (the demo's source lines), not `setXY4`'s order. The
  reverse also holds: SlicedImage::draw's tpage is exactly libgpu's `getTPage` on the RECT.
- **A signed-char field shares a constant with a compare.** RunAnim's direction byte stores `-1`.
  As `signed char`, the store reuses the register of the `from == -1` test. As `uchar`, it
  materialises 255.
- **Frame padding without `volatile`.** An unused local `char buf[40];` keeps its stack space.
- **Store position around a call can fix a register swap.** In Shatter_Face, `retval = 2;` after
  the Shatter_Glass call (not before) fixed the delay-slot fill and an s5/s6 swap.
- **Declaring a big extern table as 8 bytes or less gives retail's single `la`.** With `-G8`, cc1
  treats an extern of at most 8 bytes as possibly small and emits the `la` macro (lui and addiu in
  one register, scheduled as one insn). A larger declared size (even a sized `[4096]`) gives an
  explicit `%hi`/`%lo` split that the scheduler and reorg move apart. `extern STrig DAT_800c9670[1];`
  (the sin table) fixed CTexturedRibbon::Display, GetVecFromMagDir and CSmokeJet::Move.
  `build/t/externsweep1.py` tries `[1]` on every array extern in the near-misses.
- **CBruce's skater-stat getter is a macro, not an inline function:**
  `#define GetStat(n) (DAT_800d1924 ? DAT_800d1924 : (DAT_800a44fc[mSkaterIndex].Stats[n] + ((m254 || DAT_800d172c) ? 3 : 0)) + mpStatBonus[n])`.
  gcc 2.x `fold()` distributes the surrounding arithmetic into each `?:` arm only inside one
  expression, which gives retail's per-arm sums, threaded override tests and shared loads. Every
  inline-function form stalled near 100/313. Keep the exact operand order, and define
  `DAT_800d1924` in the TU so it is gp-relative. Used by HandleKick and DoFootPlantPhysics; other
  physics functions that read stats likely need it too.
- **More register/schedule levers from DoWallRidePhysics and HandleJump:**
  - Unused separate `CVector` locals reproduce retail's frame padding. Arrays land in the wrong slots.
  - A comparison constant held in a named local (`int CheckDist = 116; if (d <= CheckDist)`) keeps it
    in a register for `slt`.
  - Calling through a void-returning cast (`((void (*)(int))FUN_8008b5dc)(x)`) frees v0 when the
    result of an int function is ignored.
  - Duplicated calls in `if/else if` arms get cross-jumped, while `||` folds into a range check.
  - Assigning a shared tail value in each arm (`JumpBoost = …`) and applying it once outside
    reproduces retail's cross-jumped tails.
- **A scalar global read with a split `lui rA` / `lw rB,%lo(rA)`** (different registers) is an
  unsized array in the source: `extern int X[]; … X[0]`. Plain `extern int X;` gives the `lw` macro
  with one register. This fixed CPanelData::Land, M3dColij_GetLineInfo and FUN_8006d0c8.
  `build/t/scalarsweep2.py` tries it on every scalar extern in the near-misses.
- **Array walks: index and block-local pointer, not hand-made pointer bumps.**
  `for (i…) { char *n = names[i]; … }` lets loop.c create the pointer givs after the hoisted
  invariants, which is retail's preheader order (unloadPRE). Hand-written `p++` pointers are
  initialized before the hoisted `lui`.
- **Initialize, don't assign, folded `?:` expressions.** `x = <GetStat ?: expr>` goes through a
  temp register and a copy. `int x = <expr>;` builds the result directly in the variable, as
  retail does (HandleContinuousSounds: 581 → 625).
  - `Spd * 2048` keeps the `?:` arms distributed where `Spd << 11` merges them.
- **Struct copies: pad word-aligned structs with `int` arrays, not `char`.** A struct padded with
  `char[]` is byte-aligned, and its copy compiles to `lwl`/`lwr` unaligned moves. Game_Logic's
  364-byte SControl swap went from 1860 to 1148 bytes with `int b[91]`.
- **Hidden empty cases decide jump table vs compare tree.** CBaddy::ExecuteCommand's visible cases
  were dense enough for a jump table, but retail uses a compare tree. Two invisible `case N: break;`
  labels (found with `tools/casetree.py`) widen the range past gcc's 10x-count threshold.
- **`gp` access means a definition in this TU:** `CBody *DAT_800d1998;`. A `section(".data")`
  attribute or an unsized extern gives `lui` forms.
- **A comparison stored in an int** (`int ok = x != 2; if (ok)`) keeps retail's `xori`. Folded into
  an `&&` chain, it becomes a `beq`.
- **Score with the header flags.** `match_div.py` ignores `flags=` in the header, so pass
  `CC1_OPTS` (check_all/salvage do read it).

**Formerly open tooling gaps, now fixed:**
- **Switch jump tables:** both matchers place a function's own `.rdata` at its retail address.
  The address comes from `syms.json` (`".rdata": "0x…"`) or is inferred from retail's own `lui/addiu`
  pair. The table's contents (entries are `.text` labels) are then **byte-checked against retail**
  too (`.rdata @0x800a23dc: 8/8 words match retail`). The LNK reader also fixes a bug: patch
  offsets are relative to the section's *latest code record*, and a switch splits `.text` in two.
- **`nop` after `mflo`/`mfhi`:** ASPSX treats a *macro-expanded* `mflo/mfhi` like a load and adds a
  `nop` when the next instruction reads the result. A bare `mflo` gets none. The `match_div.py`
  rewrite now emits it. No ASPSX version (2.56–2.86) inserts it for bare `mflo`.
- **`lui` vs gp for a small global:** retail's `lui v1; lw v1,%lo(X)(v1)` (the same register for both)
  comes from **`extern int X;`**, i.e. declared, small and not defined in this file. cc1 then emits the
  `lw $3,X` macro and ASPSX picks `lui`. Defining it gives gp-relative access, and `extern int X[]`
  gives cc1's split `lui $7` + `lw $3`.

### Integer division — SOLVED (`tools/match_div.py`)
Retail uses **bare `div`+`mflo`** (no divide-by-zero/overflow check). cc1 emits the `div $d,$s,$t`
macro; `psyq-asm` can't produce the bare form (it drops `div $zero`/2-op forms), so division functions
go through the **real ASPSX** instead:

1. `cc1psx` → `.s`; 2. rewrite `div $d,$s,$t` → bare **2-operand** `div $s,$t` + `mflo $d`
   (`divu`/`rem`/`remu` likewise, `rem*`→`mfhi`); 3. **`ASPSX.EXE`** assembles it — no check, and it
   inserts the `lw;nop;div` load-delay nops correctly; 4. extract `.text` from the LNK object (length-
   prefixed code record) and byte-diff.

    python tools/match_div.py <src.c> <retailAddrHex> <size> [syms.json] -O2 -G0|-G8

Verified EXACT: `CVector::operator/=` (0x8004db04) 20/20, `operator/(CVector,int)` (0x8004de24) 26/26.
**Relocations are resolved too:** it parses the LNK object (code, patch, XDEF/XREF/XBSS records) and
applies MIPS26/HI16/LO16/GPREL16 patches from the same `syms.json` `match_sym.mjs` takes (the
function's `.text` goes at the target address, so local jumps resolve). All `match_sym` matches
reproduce through it, so it's the one tool that covers **div + GTE + globals/calls** together.
Limitation: the target must be the first function in the file.

### Remaining VECTOR: `operator%=`(CFriction)×2, and the rest of the
CSVector (16-bit) variants. They're matchable with the patterns above once the CFriction types are
handled.
