"""Link step of build.py: runtime, translated MIPS, generated glue, and the decompiled objects.

Generated into build/pc/gen/:
  psx_syms.ld    every `psx_<addr>` symbol the objects use, as an absolute address
  hle_table.c    pc_hle_funcs: runtime functions that implement a Psy-Q entry point
  game_table.c   pc_game_funcs: the decompiled functions (pcfn_<addr>)
  recomp.c       translated MIPS (build.py --mips only; else recomp_none.c: empty tables)
  image.bin/.s   the retail image's data, code cleared (embed_image.py), linked into thps.exe
  stubs.c        pc_stub_funcs: everything else that is a known function entry
  thunks.s       virtual-dispatch thunks and destructor-call adapters
  names.c        PC_SymbolName for logs
"""
import concurrent.futures as cf
import glob
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PC = os.path.dirname(HERE)
ROOT = os.path.dirname(os.path.dirname(PC))
EXE = os.path.join(ROOT, "extern", "thps1_retail", "SLUS_008.60")

GAME_END = 0x800858AC
LIB_END = 0x8009D0E0  # first byte of .rodata after the Psy-Q libraries
TEXT_START = 0x80010000


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if r.returncode:
        sys.stderr.write(" ".join(cmd)[:400] + "\n" + r.stdout[-3000:] + r.stderr[-6000:] + "\n")
        raise SystemExit("link: command failed")
    return r.stdout


def write_if_changed(path, text):
    if os.path.exists(path) and open(path, encoding="utf-8").read() == text:
        return
    open(path, "w", encoding="utf-8").write(text)


def jal_targets(img):
    out = set()
    for off in range(0, LIB_END - TEXT_START, 4):
        w = int.from_bytes(img[off:off + 4], "little")
        if (w >> 26) == 3:
            out.add(0x80000000 | ((w & 0x3FFFFFF) << 2))
        elif (w >> 26) == 1 and ((w >> 16) & 31) in (16, 17):  # bltzal / bgezal
            imm = w & 0xFFFF
            imm = imm - 0x10000 if imm & 0x8000 else imm
            out.add((TEXT_START + off + 4 + (imm << 2)) & 0xFFFFFFFF)
    return out


def link(ctx, metas, out, toolbin, mips=False):
    gcc = os.path.join(toolbin, "gcc.exe")
    nm = os.path.join(toolbin, "nm.exe")
    gen = os.path.join(out, "gen")
    rt = os.path.join(out, "runtime")
    os.makedirs(gen, exist_ok=True)
    os.makedirs(rt, exist_ok=True)
    sdl_cflags = ["-I" + os.path.join(os.path.dirname(toolbin), "include", "SDL2"),
                  "-I" + os.path.join(os.path.dirname(toolbin), "include"), "-Dmain=SDL_main"]
    sdl_cflags = sdl_cflags[:2]
    rt_flags = ["-m32", "-O2", "-g", "-std=gnu11", "-fno-strict-aliasing", "-fwrapv",
                "-fno-omit-frame-pointer", "-Wall", "-Wno-unused-function",
                "-Wno-unused-variable", "-Werror=implicit-function-declaration", "-I" + os.path.join(PC, "include"),
                "-I" + os.path.join(PC, "include", "psyq")] + sdl_cflags
    img = ctx.image[0x800:]

    # ---- 1. runtime -------------------------------------------------------------------------
    srcs = sorted(glob.glob(os.path.join(PC, "src", "*.c")))

    def cc(src, obj, flags):
        if os.path.exists(obj) and os.path.getmtime(obj) >= os.path.getmtime(src) and \
                all(os.path.getmtime(obj) >= os.path.getmtime(h)
                    for h in glob.glob(os.path.join(PC, "include", "**", "*.h"), recursive=True)):
            return obj
        run([gcc] + flags + ["-c", src, "-o", obj])
        return obj

    with cf.ThreadPoolExecutor() as ex:
        rt_objs = list(ex.map(lambda s: cc(s, os.path.join(rt, os.path.basename(s)[:-2] + ".o"),
                                           rt_flags), srcs))
    defined = set()
    for o in rt_objs:
        for line in run([nm, "-g", "--defined-only", o]).splitlines():
            p = line.split()
            if len(p) == 3 and p[1] == "T":
                defined.add(p[2][1:] if p[2].startswith("_") else p[2])

    # ---- 2. which addresses have which implementation ----------------------------------------
    psyq = {}
    for line in open(os.path.join(ROOT, "build", "psyq_names.tsv")):
        if line.startswith("#"):
            continue
        a, n = line.split("\t")[:2]
        psyq.setdefault(n, []).append(int(a, 16))
    hle = {}  # addr -> symbol
    override = {}
    for s in sorted(defined):
        m = re.fullmatch(r"hle_([0-9a-f]{8})", s)
        if m:
            hle[int(m.group(1), 16)] = s
            continue
        m = re.fullmatch(r"hleov_([0-9a-f]{8})", s)
        if m:
            override[int(m.group(1), 16)] = s
            continue
        name = s[4:] if s.startswith("hle_") else s
        for a in psyq.get(name, []):
            if s.startswith("hle_") or name in psyq:
                hle[a] = s
    game = {}
    for m in metas:
        for a in m.get("defines", {}):
            game[int(a, 16)] = "pcfn_%08x" % int(a, 16)

    # ---- 3. translate MIPS (build.py --mips only) ---------------------------------------------
    # A debugging build: every game function is translated too (by its known size), so any of
    # them can run as retail MIPS instead of C for A/B debugging (THPS_MIPS, see pc_main.c), and
    # so are the hand-written assembly and library functions, for THPS_SNAP and the tests. Calls
    # to game functions from translated code go through the runtime table, which honours that
    # choice. The normal build has no MIPS at all: anything without C is a stub.
    entries, sizes = {}, {}
    for a, (size, mangled, _) in ctx.functions.items():
        if not mangled.startswith("D_"):
            entries[a] = mangled
            sizes[a] = size
    lib_entries = {a for a in jal_targets(img) if GAME_END <= a < LIB_END}
    for n, addrs in psyq.items():
        for a in addrs:
            if GAME_END <= a < LIB_END:
                lib_entries.add(a)
    for m in metas:
        for s in m.get("psx_syms", []):
            a = int(s[5:13], 16)
            if GAME_END <= a < LIB_END:
                lib_entries.add(a)
    names = {}
    for n, addrs in psyq.items():
        for a in addrs:
            names[a] = n
    for a in lib_entries:
        entries.setdefault(a, names.get(a))
    native = set(hle) - set(override)  # overridden functions stay translated (wrappers call them)
    ent_file = os.path.join(gen, "recomp_entries.txt")
    nat_file = os.path.join(gen, "recomp_native.txt")
    disp_file = os.path.join(gen, "recomp_dispatch.txt")
    write_if_changed(ent_file, "".join("0x%08x %s%s\n" % (a, entries[a] or "-",
                                                          " %d" % sizes[a] if a in sizes else "")
                                       for a in sorted(entries)))
    write_if_changed(nat_file, "".join("0x%08x\n" % a for a in sorted(native)))
    write_if_changed(disp_file, "".join("0x%08x\n" % a for a in sorted(set(game) | set(override))))
    nsplit = 8
    stamp = os.path.join(gen, "recomp.stamp")
    if not mips:
        write_if_changed(os.path.join(gen, "recomp_none.c"),
                         "#include \"pc_runtime.h\"\nconst PcFunc pc_recomp_funcs[] = {{0, 0}};\n"
                         "const PcFunc pc_recomp_game_funcs[] = {{0, 0}};\n")
    elif not os.path.exists(stamp) or any(os.path.getmtime(stamp) < os.path.getmtime(p) for p in
                                          (ent_file, nat_file, disp_file,
                                           os.path.join(HERE, "mips2c.py"))):
        r = subprocess.run([sys.executable, os.path.join(HERE, "mips2c.py"), "--exe", EXE,
                            "--entries", ent_file, "--native", nat_file, "--dispatch", disp_file,
                            "--split", str(nsplit), "-o", os.path.join(gen, "recomp.c")],
                           capture_output=True, text=True)
        sys.stdout.write(r.stdout)
        sys.stderr.write(r.stderr)
        if r.returncode:
            raise SystemExit("mips2c failed")
        open(stamp, "w").write("")
    recomp_addrs = {e for e in entries if e not in native} if mips else set()

    # ---- 4. tables, stubs, thunks, names ----------------------------------------------------
    referenced = set()
    for m in metas:
        for s in m.get("psx_syms", []):
            referenced.add(int(s[5:13], 16))
    known_fn = set(ctx.functions) | lib_entries | {a for v in psyq.values() for a in v}
    code_refs = {a for a in referenced if TEXT_START <= a < LIB_END}
    stubs = sorted((known_fn | code_refs) - set(game) - set(hle) - set(override) - recomp_addrs)

    lines = ['#include "pc_runtime.h"']
    for a, s in sorted(game.items()):
        lines.append("void %s(void);" % s)
    lines.append("const PcFunc pc_game_funcs[] = {")
    lines += ["\t{0x%08xu, (void *)%s}," % (a, s) for a, s in sorted(game.items())]
    lines.append("\t{0, 0}\n};")
    write_if_changed(os.path.join(gen, "game_table.c"), "\n".join(lines) + "\n")

    lines = ['#include "pc_runtime.h"']
    allh = dict(hle)
    allh.update(override)
    for s in sorted(set(allh.values())):
        lines.append("void %s(void);" % s)
    lines.append("const PcFunc pc_hle_funcs[] = {")
    lines += ["\t{0x%08xu, (void *)%s}," % (a, s) for a, s in sorted(allh.items())]
    lines.append("\t{0, 0}\n};")
    # overrides beat decompiled C: the runtime adds them last (see pc_main.c)
    lines.append("const PcFunc pc_override_funcs[] = {")
    lines += ["\t{0x%08xu, (void *)%s}," % (a, s) for a, s in sorted(override.items())]
    lines.append("\t{0, 0}\n};")
    write_if_changed(os.path.join(gen, "hle_table.c"), "\n".join(lines) + "\n")

    lines = ['#include "pc_runtime.h"']
    for a in stubs:
        lines.append("static uint32_t stub_%08x(void) { return PC_MissingFunction(0x%08xu); }" % (a, a))
    lines.append("const PcFunc pc_stub_funcs[] = {")
    lines += ["\t{0x%08xu, (void *)stub_%08x}," % (a, a) for a in stubs]
    lines.append("\t{0, 0}\n};")
    write_if_changed(os.path.join(gen, "stubs.c"), "\n".join(lines) + "\n")

    allnames = {}
    for a, (size, mangled, dem) in ctx.functions.items():
        allnames[a] = dem
    for a, n in names.items():
        allnames.setdefault(a, n)
    lines = ['#include <stdint.h>', '#include <stdlib.h>',
             'static const struct { uint32_t a; const char *n; } T[] = {']
    for a in sorted(allnames):
        lines.append('\t{0x%08xu, "%s"},' % (a, allnames[a].replace("\\", "\\\\").replace('"', '\\"')))
    lines.append("};")
    lines.append("const char *PC_SymbolName(uint32_t a) { int lo = 0, hi = (int)(sizeof T / sizeof T[0]) - 1;"
                 " while (lo <= hi) { int m = (lo + hi) / 2; if (T[m].a == a) return T[m].n;"
                 " if (T[m].a < a) lo = m + 1; else hi = m - 1; } return NULL; }")
    write_if_changed(os.path.join(gen, "names.c"), "\n".join(lines) + "\n")

    # Thunks: virtual dispatch through the retail vtable, and destructor-call adapters.
    # Destructors (egcs): in_chrg bit 0 means "free the object afterwards". A delete statement
    # records its pointer (pc_delete_mark); the destructor call for exactly that object passes
    # in_chrg 3 and notes the object as freed, so the operator delete g++ emits next is skipped
    # (_pcdel_*). Base-class destructor calls (_pcdtorcall_*) always pass 0.
    def delete_flag():
        return ["\txorl %edx, %edx",
                "\tcmpl _pc_delete_marked, %eax",
                "\tjne 1f",
                "\tmovl %eax, _pc_delete_freed",
                "\tmovl $0, _pc_delete_marked",
                "\tmovl $3, %edx",
                "1:"]

    asm = ["\t.text"]
    done = set()
    for m in metas:
        for t in m.get("thunks", []):
            if t["key"] in done:
                continue
            done.add(t["key"])
            slot = t["slot"]
            asm += ["\t.globl _pcvt_%s" % t["key"], "_pcvt_%s:" % t["key"],
                    "\tmovl 4(%esp), %eax"]
            if t["dtor"]:
                asm += delete_flag()
            asm += ["\tmovl _%s, %%ecx" % t["off"],
                    "\tmovl (%eax,%ecx), %ecx"]
            if t["dtor"]:
                asm += ["\tpushl %edx",
                        "\tmovswl %d(%%ecx), %%edx" % (8 + 8 * slot),
                        "\taddl %edx, %eax",
                        "\tpushl %eax", "\tcall *%d(%%ecx)" % (12 + 8 * slot),
                        "\taddl $8, %esp", "\tret"]
            else:
                asm += ["\tmovswl %d(%%ecx), %%edx" % (8 + 8 * slot),
                        "\taddl %edx, %eax",
                        "\tmovl %eax, 4(%esp)", "\tjmp *%d(%%ecx)" % (12 + 8 * slot)]
        for a in m.get("dtor_calls", []):
            if ("d", a) in done:
                continue
            done.add(("d", a))
            asm += ["\t.globl _pcdtorcall_%08x" % a, "_pcdtorcall_%08x:" % a,
                    "\tpushl $0", "\tpushl 8(%esp)", "\tmovl $0x%08x, %%eax" % a,
                    "\tcall *%eax", "\taddl $8, %esp", "\tret"]
        for a in m.get("dtor_calls_d", []):
            if ("dd", a) in done:
                continue
            done.add(("dd", a))
            asm += ["\t.globl _pcdtorcalld_%08x" % a, "_pcdtorcalld_%08x:" % a,
                    "\tmovl 4(%esp), %eax"] + delete_flag() + [
                    "\tpushl %edx", "\tpushl %eax", "\tmovl $0x%08x, %%eax" % a,
                    "\tcall *%eax", "\taddl $8, %esp", "\tret"]
        for a in m.get("del_wrappers", []):
            if ("del", a) in done:
                continue
            done.add(("del", a))
            asm += ["\t.globl _pcdel_%08x" % a, "_pcdel_%08x:" % a,
                    "\tmovl 4(%esp), %eax",
                    "\tcmpl _pc_delete_freed, %eax",
                    "\tjne 1f",
                    "\tmovl $0, _pc_delete_freed",
                    "\tret",
                    "1:",
                    "\tmovl $0, _pc_delete_marked",
                    "\tmovl $0x%08x, %%eax" % a,
                    "\tjmp *%eax"]
    # constructors defined in C++: call the native one with the caller's argument words, then
    # return `this` as egcs constructors do
    for m in metas:
        for a in m.get("ctor_defs", []):
            if ("ctor", a) in done:
                continue
            done.add(("ctor", a))
            asm += ["\t.globl _pcfn_%08x" % a, "_pcfn_%08x:" % a,
                    "\tpushl %ebx",
                    "\tpushl %esi",
                    "\tpushl %edi",
                    "\tmovl 16(%esp), %ebx",          # this
                    "\tsubl $48, %esp",
                    "\tleal 64(%esp), %esi",          # first argument word
                    "\tmovl %esp, %edi",
                    "\tmovl $12, %ecx",
                    "\tcld",
                    "\trep movsl",
                    "\tcall _pcctor_%08x" % a,
                    "\taddl $48, %esp",
                    "\tmovl %ebx, %eax",
                    "\tpopl %edi",
                    "\tpopl %esi",
                    "\tpopl %ebx",
                    "\tret"]
    # destructors defined in C++: the egcs destructor is (this, in_chrg); run the native
    # base-object destructor (body, then the base classes'), then free if in_chrg bit 0 is set
    for m in metas:
        for a, cls, dl in m.get("dtor_defs", []):
            if ("dtor", a) in done:
                continue
            done.add(("dtor", a))
            asm += ["\t.globl _pcfn_%08x" % a, "_pcfn_%08x:" % a,
                    "\tpushl 4(%esp)",
                    "\tcall _pcdtor_%08x" % a,
                    "\taddl $4, %esp",
                    "\ttestb $1, 8(%esp)",
                    "\tjz 1f"]
            if not dl:
                raise SystemExit("no operator delete for %s (destructor 0x%08x)" % (cls, a))
            asm += ["\tmovl $0x%08x, %%eax" % dl, "\tjmp *%eax", "1:", "\tret"]
    # extra names objcopy needed when two symbols in one object map to the same thunk
    aliases = set()
    for m in metas:
        aliases.update(m.get("aliases", []))
    for al in sorted(aliases):
        base = re.sub(r"_a\d+$", "", al)
        if base + ":" in asm:
            k = asm.index(base + ":")
            asm[k:k] = ["\t.globl %s" % al, "%s:" % al]
    write_if_changed(os.path.join(gen, "thunks.s"), "\n".join(asm) + "\n")

    syms = set()
    for m in metas:
        syms.update(m.get("psx_syms", []))
    ld = "".join("%s = 0x%s;\n" % (s, s[5:13]) for s in sorted(syms))
    write_if_changed(os.path.join(gen, "psx_syms.ld"), ld)

    # ---- 5. compile generated sources and link ------------------------------------------------
    gen_objs = []
    gflags = rt_flags + ["-O1", "-Wno-unused-label", "-Wno-unused-but-set-variable",
                         "-Wno-maybe-uninitialized"]
    jobs = [(os.path.join(gen, n), os.path.join(gen, n[:-2] + ".o"))
            for n in ["game_table.c", "hle_table.c", "stubs.c", "names.c"] +
            (["recomp_%d.c" % k for k in range(nsplit)] if mips else ["recomp_none.c"])]
    with cf.ThreadPoolExecutor() as ex:
        gen_objs = list(ex.map(lambda j: cc(j[0], j[1], gflags), jobs))
    th_o = os.path.join(gen, "thunks.o")
    run([gcc, "-m32", "-c", os.path.join(gen, "thunks.s"), "-o", th_o])
    gen_objs.append(th_o)

    # the retail image's data (its code cleared), copied to 0x80010000 at startup (pc_main.c)
    img_bin = os.path.join(gen, "image.bin")
    if not os.path.exists(img_bin) or any(os.path.getmtime(img_bin) < os.path.getmtime(p) for p in
                                          (EXE, os.path.join(HERE, "embed_image.py"),
                                           os.path.join(ROOT, "decomp", "functions.tsv"))):
        sys.stdout.write(run([sys.executable, os.path.join(HERE, "embed_image.py"), EXE,
                              img_bin]))
    write_if_changed(os.path.join(gen, "image.s"),
                     "\t.section .rdata,\"dr\"\n\t.p2align 4\n\t.globl _pc_image\n_pc_image:\n"
                     "\t.incbin \"image.bin\"\n\t.globl _pc_image_end\n_pc_image_end:\n")
    img_o = os.path.join(gen, "image.o")
    if not os.path.exists(img_o) or os.path.getmtime(img_o) < os.path.getmtime(img_bin):
        run([gcc, "-m32", "-c", os.path.join(gen, "image.s"), "-o", img_o], cwd=gen)
    gen_objs.append(img_o)

    # application icon: SDL2 gives the window the exe's first icon, so this also sets the taskbar
    # and title-bar icon
    ico = os.path.join(ROOT, "icon.ico")
    write_if_changed(os.path.join(gen, "thps.rc"),
                     '1 ICON "%s"\n' % ico.replace("\\", "/"))
    res_o = os.path.join(gen, "thps_res.o")
    if not os.path.exists(res_o) or os.path.getmtime(res_o) < max(
            os.path.getmtime(ico), os.path.getmtime(os.path.join(gen, "thps.rc"))):
        run([os.path.join(toolbin, "windres.exe"), "-F", "pe-i386",
             os.path.join(gen, "thps.rc"), "-o", res_o])
    gen_objs.append(res_o)

    game_objs = [os.path.join(out, "obj", os.path.splitext(m["src"])[0] + ".o") for m in metas]
    rsp = os.path.join(gen, "objs.rsp")
    open(rsp, "w").write("\n".join('"%s"' % o.replace("\\", "/") for o in
                                   rt_objs + gen_objs + game_objs) + "\n")
    exe = os.path.join(out, "thps.exe")
    libs = ["-lmingw32", "-lSDL2", "-lm", "-ldinput8", "-ldxguid", "-ldxerr8",
            "-luser32", "-lgdi32", "-lwinmm", "-limm32", "-lole32", "-loleaut32", "-lshell32",
            "-lsetupapi", "-lversion", "-luuid", "-lopengl32", "-static"]
    run([gcc, "-m32", "-g", "@" + rsp, "-o", exe, "-mconsole",
         "-Wl,--large-address-aware", "-Wl,--disable-dynamicbase", "-Wl,--image-base=0x400000",
         "-Wl," + os.path.join(gen, "psx_syms.ld").replace("\\", "/")] + libs)
    print("linked %s: %d decompiled, %d HLE, %d translated, %d stubs%s" % (
        exe, len(game), len(allh), len(recomp_addrs), len(stubs),
        "" if mips else " (no MIPS: build.py --mips translates the retail code for debugging)"))
    return 0
