"""Link step of build.py: runtime, generated glue, and the game's objects.

Generated into build/pc/gen/:
  psx_syms.ld    every `psx_<addr>` symbol: a function's native code (pcfn_<addr>, or its stub),
                 a global's place in the native data, else (.bss) its address in PS1 RAM
  data.s/.bin    the game's data (game/data, data_image.py), native, in its retail layout
  data_table.c   pc_data: where the native data is, by retail address (PC_Data)
  stubs.c        pcstub_<addr>: every other known function entry (logs a call)
  thunks.s       virtual-dispatch thunks and destructor-call adapters
  names.c        PC_SymbolName for logs
"""
import bisect
import concurrent.futures as cf
import glob
import io
import json
import os
import re
import shlex
import subprocess
import sys
import tarfile
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from toolchain import EXE_NAME, USCORE, WINDOWS, tool  # noqa: E402
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


LINUX_NOTICES_HEAD = """Third-party software in thps
============================

thps has these libraries built into it. Their copyright and license notices follow.

  PL_MPEG               MIT license             https://github.com/phoboslab/pl_mpeg
  glad (OpenGL loader)  CC0-1.0 / WTFPL, with   https://github.com/Dav1dde/glad
                        Khronos API data under
                        Apache-2.0
  GCC runtime (libgcc)  GPL-3.0 with the GCC Runtime Library Exception (no notice required)

It also uses the system's SDL 2 (zlib license, https://libsdl.org), C library and OpenGL
driver, which are not part of this download.

"""


def package(out, toolbin):
    """The release download, build/pc/release/thps-windows.zip: thps.exe, the player's README,
    the project's LICENSE and the third-party notices (the SDL2 and MinGW-w64 runtime ones from
    the toolchain that was linked). On Linux, build/pc-linux/release/thps-linux.tar.gz: thps,
    dist/README-linux.txt as README.txt, the LICENSE, and the notices of what is built in (SDL2
    and the C library are the system's)."""
    dist = os.path.join(PC, "dist")
    lic = os.path.join(os.path.dirname(toolbin), "share", "licenses")
    notices = [open(os.path.join(dist, "THIRD-PARTY-NOTICES.txt"), encoding="utf-8").read()]
    if WINDOWS:
        extra = [("SDL 2", os.path.join(lic, "SDL2", "LICENSE.txt")),
                 ("MinGW-w64 runtime", os.path.join(lic, "crt", "COPYING.MinGW-w64-runtime.txt"))]
    else:
        # the notices file's head lists the Windows build's contents
        notices[0] = LINUX_NOTICES_HEAD + notices[0][notices[0].index("-" * 96):]
        extra = []
    for title, path in extra + [("Apache License 2.0",
                                 os.path.join(dist, "licenses", "Apache-2.0.txt"))]:
        notices.append("\n\n%s\n%s\n%s\n\n%s" % ("-" * 96, title, "-" * 96,
                                                  open(path, encoding="utf-8").read().strip()))
    readme = "README.txt" if WINDOWS else "README-linux.txt"
    files = {EXE_NAME: open(os.path.join(out, EXE_NAME), "rb").read(),
             "README.txt": open(os.path.join(dist, readme), "rb").read(),
             "THIRD-PARTY-NOTICES.txt": "".join(notices).replace("\r\n", "\n").encode("utf-8")}
    for name in ("LICENSE", "LICENSE.txt", "LICENSE.md"):
        if os.path.exists(os.path.join(ROOT, name)):
            files["LICENSE.txt"] = open(os.path.join(ROOT, name), "rb").read()
            break
    else:
        print("package: no LICENSE in the repository root; the download has none")
    for name, data in files.items():
        if name != EXE_NAME:
            data = data.replace(b"\r\n", b"\n")
            if WINDOWS:
                data = data.replace(b"\n", b"\r\n")  # Windows line endings for Notepad
            open(os.path.join(out, name), "wb").write(data)
            files[name] = data
    if WINDOWS:
        zpath = os.path.join(out, "thps-windows.zip")
        with zipfile.ZipFile(zpath + ".tmp", "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
            for name, data in files.items():
                z.writestr(name, data)
    else:
        zpath = os.path.join(out, "thps-linux.tar.gz")
        with tarfile.open(zpath + ".tmp", "w:gz", compresslevel=9) as z:
            for name, data in files.items():
                info = tarfile.TarInfo(name)
                info.size = len(data)
                info.mode = 0o755 if name == EXE_NAME else 0o644
                z.addfile(info, io.BytesIO(data))
    os.replace(zpath + ".tmp", zpath)
    print("packaged %s: %s" % (zpath, ", ".join(files)))


def sdl_flags():
    """(compile flags, link flags) for SDL2: the MinGW toolchain's on Windows; elsewhere the 32-bit
    one pkg-config knows (THPS_SDL_CFLAGS / THPS_SDL_LIBS override)."""
    if WINDOWS:
        return None
    cflags, libs = os.environ.get("THPS_SDL_CFLAGS"), os.environ.get("THPS_SDL_LIBS")
    if cflags is None or libs is None:
        env = dict(os.environ)
        # Debian/Ubuntu multiarch and Fedora/Arch lib32 locations of the i386 .pc files
        dirs = [d for d in ("/usr/lib/i386-linux-gnu/pkgconfig", "/usr/lib32/pkgconfig",
                            "/usr/lib/pkgconfig") if os.path.isdir(d)]
        if dirs and "PKG_CONFIG_LIBDIR" not in env:
            env["PKG_CONFIG_LIBDIR"] = os.pathsep.join(dirs + ["/usr/share/pkgconfig"])
        try:
            if cflags is None:
                cflags = subprocess.run(["pkg-config", "--cflags", "sdl2"], env=env, check=True,
                                        capture_output=True, text=True).stdout
            if libs is None:
                libs = subprocess.run(["pkg-config", "--libs", "sdl2"], env=env, check=True,
                                      capture_output=True, text=True).stdout
        except (OSError, subprocess.CalledProcessError):
            cflags = cflags if cflags is not None else "-I/usr/include/SDL2 -D_REENTRANT"
            libs = libs if libs is not None else "-lSDL2"
    return shlex.split(cflags), shlex.split(libs)


def link(ctx, metas, out, toolbin, release=False):
    gcc = tool("gcc")
    objroot = out
    if release:
        out = os.path.join(out, "release")
    nm = tool("nm")
    gen = os.path.join(out, "gen")
    rt = os.path.join(out, "runtime")
    os.makedirs(gen, exist_ok=True)
    os.makedirs(rt, exist_ok=True)
    if WINDOWS:
        sdl_cflags = ["-I" + os.path.join(os.path.dirname(toolbin), "include", "SDL2"),
                      "-I" + os.path.join(os.path.dirname(toolbin), "include")]
        sdl_libs = []
    else:
        sdl_cflags, sdl_libs = sdl_flags()
    rt_flags = ["-m32", "-O2", "-g", "-std=gnu11", "-fno-strict-aliasing", "-fwrapv",
                "-fno-omit-frame-pointer", "-Wall", "-Wno-unused-function",
                "-Wno-unused-variable", "-Werror=implicit-function-declaration", "-I" + os.path.join(PC, "include"),
                "-I" + os.path.join(PC, "include", "psyq")] + sdl_cflags
    if not WINDOWS:
        # absolute addresses (no PIE), the Windows ABI's small-struct returns (as the game code,
        # build.py), 64-bit file offsets (disc images are bigger than 2 GB)
        rt_flags += ["-fno-pic", "-fno-pie", "-freg-struct-return", "-D_FILE_OFFSET_BITS=64"]
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
                defined.add(p[2][len(USCORE):] if p[2].startswith(USCORE) else p[2])

    # ---- 2. which addresses have which implementation ----------------------------------------
    psyq = {}
    for line in open(os.path.join(ROOT, "build", "psyq_names.tsv")):
        if line.startswith("#"):
            continue
        a, n = line.split("\t")[:2]
        psyq.setdefault(n, []).append(int(a, 16))
    for s in sorted(defined):
        if re.fullmatch(r"hle(ov)?_([0-9a-f]{8})", s):
            # the port doesn't implement or replace functions at retail addresses
            raise SystemExit("%s: the game's code calls native services directly; nothing "
                             "replaces a function at its retail address" % s)
    game = {}
    for m in metas:
        for a in m.get("defines", {}):
            game[int(a, 16)] = "pcfn_%08x" % int(a, 16)

    # ---- 3. the Psy-Q library entries the game calls (stubs, names) -------------------------
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
    # The game's data (game/data; data_image.py checks it against the retail data) is native:
    # data.s, its globals and .bss in their retail layout with native pointers and string literals.
    img_bin = os.path.join(gen, "image.bin")
    data_s = os.path.join(gen, "data.s")
    r = subprocess.run([sys.executable, os.path.join(HERE, "data_image.py"), img_bin,
                        "--check", EXE, "--asm", data_s], capture_output=True, text=True)
    sys.stdout.write(r.stdout)
    if r.returncode:
        sys.stderr.write(r.stderr)
        raise SystemExit("link: game/data doesn't build the retail data (data_image.py)")
    data = json.load(open(data_s[:-2] + ".json"))
    if data["fixed"]:
        raise SystemExit("link: game/data/fixed has globals, but nothing is at a PS1 address any "
                         "more: make them native data")

    # ---- 4. tables, stubs, thunks, names ----------------------------------------------------
    # every code address the game's code or data names: a function, else a stub (library tables
    # in the data point at library internals the port doesn't have)
    referenced = set(data["refs"])
    for m in metas:
        for s in m.get("psx_syms", []):
            referenced.add(int(s[5:13], 16))
    known_fn = set(ctx.functions) | lib_entries | {a for v in psyq.values() for a in v}
    code_refs = {a for a in referenced if TEXT_START <= a < LIB_END}
    stubs = sorted((known_fn | code_refs) - set(game))

    lines = ['#include "pc_runtime.h"']
    for a in stubs:
        lines.append("uint32_t pcstub_%08x(void) { return PC_MissingFunction(0x%08xu); }" % (a, a))
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

    # libsn's operator delete freed into the BIOS heap, which the game never allocates from: on
    # PC it is the native pc_delete_unowned (build.py NATIVE_RENAME)
    unowned = psyq.get("__builtin_delete", [None])[0]

    def jump_to(a):
        if a == unowned:
            return ["\tjmp _pc_delete_unowned"]
        return ["\tjmp _psx_%08x" % a]

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
                    "\tpushl $0", "\tpushl 8(%esp)", "\tcall _psx_%08x" % a,
                    "\taddl $8, %esp", "\tret"]
        for a in m.get("dtor_calls_d", []):
            if ("dd", a) in done:
                continue
            done.add(("dd", a))
            asm += ["\t.globl _pcdtorcalld_%08x" % a, "_pcdtorcalld_%08x:" % a,
                    "\tmovl 4(%esp), %eax"] + delete_flag() + [
                    "\tpushl %edx", "\tpushl %eax", "\tcall _psx_%08x" % a,
                    "\taddl $8, %esp", "\tret"]
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
                    "\tmovl $0, _pc_delete_marked"] + jump_to(a)
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
            asm += jump_to(dl) + ["1:", "\tret"]
    # extra names objcopy needed when two symbols in one object map to the same thunk
    aliases = set()
    for m in metas:
        aliases.update(m.get("aliases", []))
    for al in sorted(aliases):
        base = re.sub(r"_a\d+$", "", al)
        if base + ":" in asm:
            k = asm.index(base + ":")
            asm[k:k] = ["\t.globl %s" % al, "%s:" % al]
    asm_text = "\n".join(asm) + "\n"
    if not WINDOWS:
        # the names above are PE's (`_pcvt_...`, `_pc_delete_marked`); ELF has no '_' prefix
        asm_text = re.sub(r"(?<![\w$.])_(?=pc|psx_)", "", asm_text)
        asm_text += "\t.section .note.GNU-stack,\"\",@progbits\n"
    write_if_changed(os.path.join(gen, "thunks.s"), asm_text)

    # psx_<addr>: what the game's code (and the data) refers to by retail address. A function is
    # its native code (or stub); in a block of native data, it's its place there (a global, or a
    # field or element the code names as a global of its own); anything else (.bss) is in PS1
    # RAM, at that address. (The TU metadata spells them as PE does, `_psx_80012345`,
    # `_psx_80012345_a1` for a second name objcopy needed.)
    syms = {"_psx_%08x" % a for a in data["refs"]}
    syms.update("_psx_%08x" % a for a in list(game) + stubs)
    for m in metas:
        syms.update(m.get("psx_syms", []))
    starts = [b[0] for b in data["blocks"]]
    fixed = data["fixed"]

    def native_block(a):
        k = bisect.bisect_right(starts, a) - 1
        if k < 0 or a >= data["blocks"][k][1] or any(s <= a < e for s, e in fixed):
            return None
        return starts[k]

    ld, dead = [], []
    stub_set = set(stubs)
    for s in sorted(syms):
        a = int(s[5:13], 16)
        b = native_block(a)
        name = s if WINDOWS else s[1:]
        if a in game or a in stub_set:
            ld.append("%s = %s%s_%08x;\n" % (name, USCORE, "pcfn" if a in game else "pcstub", a))
        elif TEXT_START <= a < LIB_END and b is None:
            raise SystemExit("link: %s is in the retail code but no function" % s)
        elif b is None:
            # outside the game's data: a library table's pointer into library data the port
            # doesn't have, never followed
            dead.append(a)
            ld.append("%s = 0x%08x;\n" % (name, a))
        else:
            ld.append("%s = %spc_data_%08x + %d;\n" % (name, USCORE, b, a - b))
    write_if_changed(os.path.join(gen, "psx_syms.ld"), "".join(ld))
    if dead:
        print("link: %d addresses outside the game's data (never followed): %s" % (
            len(dead), " ".join("%08x" % a for a in sorted(dead))))
    # PC_Data's table: the blocks, less the fixed data inside them
    ranges = []
    for s, e in data["blocks"]:
        cuts = sorted((max(fs, s), min(fe, e)) for fs, fe in fixed if fs < e and fe > s)
        a = s
        for cs, ce in cuts + [(e, e)]:
            if cs > a:
                ranges.append((a, cs, s))
            a = max(a, ce)
    lines = ['#include "pc_runtime.h"']
    lines += ["extern char pc_data_%08x[];" % b[0] for b in data["blocks"]]
    lines.append("const PcData pc_data[] = {")
    lines += ["\t{0x%08xu, 0x%08xu, pc_data_%08x + %d}," % (a, e, b, a - b) for a, e, b in ranges]
    lines.append("\t{0, 0, 0}\n};")
    write_if_changed(os.path.join(gen, "data_table.c"), "\n".join(lines) + "\n")

    # ---- 5. compile generated sources and link ------------------------------------------------
    gen_objs = []
    gflags = rt_flags + ["-O1", "-Wno-unused-label", "-Wno-unused-but-set-variable",
                         "-Wno-maybe-uninitialized"]
    jobs = [(os.path.join(gen, n), os.path.join(gen, n[:-2] + ".o"))
            for n in ["stubs.c", "names.c", "data_table.c"]]
    with cf.ThreadPoolExecutor() as ex:
        gen_objs = list(ex.map(lambda j: cc(j[0], j[1], gflags), jobs))
    th_o = os.path.join(gen, "thunks.o")
    run([gcc, "-m32", "-c", os.path.join(gen, "thunks.s"), "-o", th_o])
    gen_objs.append(th_o)
    data_o = os.path.join(gen, "data.o")
    if not os.path.exists(data_o) or os.path.getmtime(data_o) < max(
            os.path.getmtime(data_s), os.path.getmtime(data_s[:-2] + ".bin")):
        run([gcc, "-m32", "-c", data_s, "-o", data_o], cwd=gen)
    gen_objs.append(data_o)


    if WINDOWS:
        # application icon: SDL2 gives the window the exe's first icon, so this also sets the
        # taskbar and title-bar icon
        ico = os.path.join(ROOT, "icon.ico")
        write_if_changed(os.path.join(gen, "thps.rc"),
                         '1 ICON "%s"\n' % ico.replace("\\", "/"))
        res_o = os.path.join(gen, "thps_res.o")
        if not os.path.exists(res_o) or os.path.getmtime(res_o) < max(
                os.path.getmtime(ico), os.path.getmtime(os.path.join(gen, "thps.rc"))):
            run([tool("windres"), "-F", "pe-i386",
                 os.path.join(gen, "thps.rc"), "-o", res_o])
        gen_objs.append(res_o)

    game_objs = [os.path.join(objroot, "obj", os.path.splitext(m["src"])[0] + ".o") for m in metas]
    rsp = os.path.join(gen, "objs.rsp")
    open(rsp, "w").write("\n".join('"%s"' % o.replace("\\", "/") for o in
                                   rt_objs + gen_objs + game_objs) + "\n")
    exe = os.path.join(out, EXE_NAME)
    if WINDOWS:
        # everything but Windows itself is linked in (SDL2, libgcc, libstdc++, winpthreads)
        libs = ["-lmingw32", "-lSDL2", "-lm", "-ldinput8", "-ldxguid", "-ldxerr8",
                "-luser32", "-lgdi32", "-lwinmm", "-limm32", "-lole32", "-loleaut32", "-lshell32",
                "-lsetupapi", "-lversion", "-luuid", "-lopengl32", "-static"]
        # release: a GUI program (no console window; the log goes to thps.log, pc_main.c)
        ldflags = ["-mwindows" if release else "-mconsole", "-Wl,--large-address-aware",
                   "-Wl,--disable-dynamicbase", "-Wl,--image-base=0x400000"]
    else:
        # SDL2 and the C library are the system's; OpenGL comes through SDL. Not position
        # independent: the image sits at its default 0x08048000
        libs = sdl_libs + ["-lm", "-lpthread"]
        ldflags = ["-no-pie", "-Wl,-z,noexecstack"]
    run([gcc, "-m32", "-g", "@" + rsp, "-o", exe] + ldflags +
        ["-Wl," + os.path.join(gen, "psx_syms.ld").replace("\\", "/")] + libs)
    if release:
        # the debug info goes to thps.debug (for symbolize.py; not shipped)
        dbg = os.path.join(out, "thps.debug")
        run([tool("objcopy"), "--only-keep-debug", exe, dbg])
        run([tool("strip"), "--strip-all", exe])
        run([tool("objcopy"), "--add-gnu-debuglink=thps.debug", exe], cwd=out)
        package(out, toolbin)
    print("linked %s: %d game functions, %d stubs" % (exe, len(game), len(stubs)))
    return 0
