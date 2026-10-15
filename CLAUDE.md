# Tony Hawk's Pro Skater (PS1) — decompilation toward a Windows port

Long-term goal: a **playable THPS1 on Windows**. Approach is **not yet committed** — Phase 1
(this) is tooling + a coverage/scoping analysis to choose decompilation vs. static recompilation
with real numbers. See `docs/coverage-report.md` for the findings.

This is a MIPS R3000 (PS1) target. The Hydro Thunder project (`D:\Decompilation\Hydro Thunder`)
is the tooling-style reference, but its "run the original x86 binary in-process + HLE" trick does
**not** transfer to MIPS; here the port will be either RecompOne-style static recompilation or a
matching decompilation, decided after the Phase-2 spike.

## Assets (git-ignored, copyrighted — never commit)

| what | path | notes |
|---|---|---|
| THPS2 **demo** (early, symbols) | `extern/demo/PROSKATE.EXE` + `extern/demo/PROSKATE/MAIN.SYM` | text @ `0x80018000`, US region; the Rosetta Stone |
| THPS1 **retail** (target) | `extern/thps1_retail/SLUS_008.60` | text @ `0x80010000`, entry `0x80091f9c`, no symbols |
| source discs | `Euro Demo 07-00 …(Track 1/2).bin`, `Tony Hawk's Pro Skater (USA).chd`, `…2 (USA).chd` | demo disc is multi-game; THPS is under `PROSKATE/` |

`MAIN.SYM` is a **full Psy-Q debug** file: 1,645 functions (all typed), 156,874 C type
definitions, 98 source files (`C:\psx\sk2demo\*.CPP`), full source-line data. Its function
addresses all fall inside `PROSKATE.EXE`.

## Installed tooling (outside the repo)

| tool | location | purpose |
|---|---|---|
| Ghidra 12.0.3 | `D:\Tools\ghidra` | RE (shared with Hydro) |
| `ghidra_psx_ldr` 12.0.3 | `D:\Tools\ghidra\Ghidra\Extensions\ghidra_psx_ldr` | PSX-EXE loader (class `PsxLoader`), Psy-Q `.SYM`/types, GTE, `.LIB` signatures |
| GhidraMCP 5.3.1 (xebyte) | `D:\Tools\ghidra\bridge_mcp_ghidra.py`; per-user plugin in `%APPDATA%\ghidra\…\Extensions\GhidraMCP` | MCP bridge; **port 8090** here (Hydro is 8089) |
| dumpsxiso / mkpsxiso 2.30 | `D:\Tools\mkpsxiso` | extract files from PS1 discs |
| Psy-Q 4.5 SDK headers | `D:\Tools\psyq4.5\include` | `LIBGTE.H`/`INLINE_C.H` etc. for GTE matching (copyrighted; see `decomp/README.md`) |
| chdman 0.251 | `D:\Tools\chdman\chdman.exe` | CHD → BIN/CUE |
| Python 3.13 | `…\Python313\python` | parsers/analysis |

## Ghidra + MCP

- Project: `ghidra\THPS.gpr`. Programs imported: `/PROSKATE.EXE` (demo), later `/SLUS_008.60`.
- Import headless: `scripts\build_project.ps1 -Program <tag> -Exe <path> [-Tsv <names.tsv>] -Force`.
  Key gotcha: headless `-loader` wants the **class name `PsxLoader`**, not the display name; the
  loader then auto-picks language `PSX:LE:32:default`. `analyzeHeadless.bat` caps memory at 2G so
  the script calls `support\launch.bat` directly (as Hydro does).
- Serve for MCP without the GUI: `scripts\start_headless_mcp.ps1 [-ProgramPath /PROSKATE.EXE]`,
  `scripts\stop_headless_mcp.ps1`. Or open the project in the Ghidra GUI (the GhidraMCP plugin
  serves 8090). Only one holder at a time. `python scripts\mcp_smoke_test.py` checks the chain.
- Applying symbols headless: `ghidra_scripts\ApplyPsxSym.java <tsv>` (TSV = `addr size F|D name`).
  The loader's own `ghidra_scripts\ImportPSX_SYM.java` applies the full 157k **types** too but uses
  a GUI file picker — run it from the GUI when you want types, not just names.

## Analysis tooling (`tools/`)

- `parse_psyq_sym.py <MAIN.SYM>` — exact Psy-Q `.SYM` parser (format mirrored from `ImportPSX_SYM`);
  writes `build/MAIN_sym.json` and prints the census/module map.
- `match_functions.py` — normalises MIPS (masks j/jal targets + lui/addiu/ori/load-store imm16) and
  finds demo functions byte-identical in retail; writes `build/matches.json`. The demo→THPS1
  correlation (474 exact / 31.7%, a lower bound).
- `emit_ghidra_tsv.py` — turns the two JSONs into `build/demo_syms.tsv` and `build/retail_names.tsv`
  (demo names ported to retail addresses).
- `trg_dump.py <file.trg>` / `--check` — disassembles a level `.TRG` (nodes, trigger command lists,
  object scripts) with the opcode names in `game/include/game/script.h`; `--check` over
  `build/pc/data/Scripts/*.trg` must report 0 errors after any operand-format change.

## Naming conventions

Symbols are GNU-mangled C++ from the demo (e.g. `TempTransform__6CBruce`, `GetFaceFlags__FP9SLineInfo`).
`CBruce` is the skater/physics class (Neversoft codename "Bruce"). Modules are the `sk2demo` source
files: `PHYSICS/M3D/M3DCOLIJ` (physics/3D/collision), `TRIG/VECTOR/QUAT` (math), `SPOOL/SPOOLER/REDBOOK`
(CD), `CARD` (memcard), `PAD` (controller), `FRONT/MENU/PANEL` (UI), `RAIL/TRICKOB` (grind/trick).

## Repo hygiene

`extern/` and `build/` are git-ignored; disc images and extracted executables are copyrighted and
**must never be committed**. When a public port tree exists it will follow Hydro's private/public
split (RE tooling private via `.git/info/exclude`; only the port + docs public).
