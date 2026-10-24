"""Link step of build.py: runtime, generated glue, and the game's objects and data.

Generated into build/pc/gen/:
  psx_syms.ld    every `psx_<addr>` symbol (a function the code or the data names by its retail
                 address): its native code (pcfn_<addr>), or its stub
  stubs.c        pcstub_<addr>: every other known function entry (logs a call)
  thunks.c       virtual-dispatch thunks and destructor-call adapters (plain C)
  names.c        PC_SymbolName for logs
  data_names.c   PC_DataByName: the game's data (game/data) by name, for THPS_POKE
"""
import concurrent.futures as cf
import glob
import io
import os
import re
import shlex
import subprocess
import sys
import tarfile
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from toolchain import EXE_NAME, MFLAG, SANITIZE, USCORE, WINDOWS, X64, tool  # noqa: E402
PC = os.path.dirname(HERE)
ROOT = os.path.dirname(os.path.dirname(PC))

# the runtime's C++: no exceptions or RTTI (as the game's code); -Wno-missing-field-initializers
# for the `= {0}` zero-initialisers
CXX_RT_FLAGS = ["-std=gnu++20", "-fno-exceptions", "-fno-rtti", "-Wno-missing-field-initializers"]

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


def build_version(version):
    """The build's version (build.py --version; None: "dev") as (the version, the full string with
    the commit it was built from, the 4 numbers of a Windows version resource). The commit is
    best effort: "+dirty" when tracked files differ from it, THPS_COMMIT when set (the Linux
    build's container has no git: build-linux.bat passes it), left out when git can't tell."""
    version = version or "dev"
    nums = [int(n) for n in re.match(r"[\d.]*", version).group(0).split(".") if n]
    if any(n > 0xFFFF for n in nums):
        raise SystemExit("link: --version %s: each number must be at most 65535" % version)
    nums = (nums + [0, 0, 0, 0])[:4]
    commit = os.environ.get("THPS_COMMIT")
    if not commit:
        try:
            git = ["git", "-C", ROOT]
            commit = subprocess.run(git + ["rev-parse", "--short=8", "HEAD"], capture_output=True,
                                    text=True, check=True).stdout.strip()
            if subprocess.run(git + ["status", "--porcelain", "--untracked-files=no"],
                              capture_output=True, text=True, check=True).stdout.strip():
                commit += "+dirty"
        except (OSError, subprocess.CalledProcessError):
            commit = None
    return version, "%s (%s)" % (version, commit) if commit else version, nums


def version_rc(version, full, nums):
    """The Windows version resource (the exe's Properties > Details)."""
    n = ",".join(str(x) for x in nums)
    strings = [("FileDescription", "Tony Hawk's Pro Skater"), ("FileVersion", full),
               ("InternalName", "thps"), ("OriginalFilename", EXE_NAME),
               ("ProductName", "Tony Hawk's Pro Skater"), ("ProductVersion", version)]
    return ("1 VERSIONINFO\nFILEVERSION %s\nPRODUCTVERSION %s\nFILEFLAGSMASK 0x3f\nFILEFLAGS 0\n"
            "FILEOS 0x40004\nFILETYPE 1\nFILESUBTYPE 0\nBEGIN\n"
            "  BLOCK \"StringFileInfo\"\n  BEGIN\n    BLOCK \"040904b0\"\n    BEGIN\n%s"
            "    END\n  END\n"
            "  BLOCK \"VarFileInfo\"\n  BEGIN\n    VALUE \"Translation\", 0x409, 1200\n  END\n"
            "END\n") % (n, n, "".join('      VALUE "%s", "%s"\n' % kv for kv in strings))


LINUX_NOTICES_HEAD = """Third-party software in thps
============================

thps has these libraries built into it. Their copyright and license notices follow.

  PL_MPEG               MIT license             https://github.com/phoboslab/pl_mpeg
  glad (OpenGL loader)  CC0-1.0 / WTFPL, with   https://github.com/Dav1dde/glad
                        Khronos API data under
                        Apache-2.0
  GCC runtime (libgcc,  GPL-3.0 with the GCC Runtime Library Exception (no notice required)
  libstdc++)

It also uses the system's SDL 2 (zlib license, https://libsdl.org), C library and OpenGL
driver, which are not part of this download.

"""


def package(out, toolbin, version=None):
    """The release download, build/pc/release/thps-windows.zip (thps-windows-<version>.zip with
    build.py --version, the same for Linux's archive): thps.exe, the player's README,
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
    suffix = "-" + version if version else ""
    if WINDOWS:
        zpath = os.path.join(out, "thps-windows%s.zip" % suffix)
        with zipfile.ZipFile(zpath + ".tmp", "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
            for name, data in files.items():
                z.writestr(name, data)
    else:
        zpath = os.path.join(out, "thps-linux%s.tar.gz" % suffix)
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


# the parameter types a thunk forwards as one word each (intptr_t): scalars and pointers
WORD_TYPES = {"bool", "char", "signed char", "unsigned char", "short", "short int",
              "signed short", "signed short int", "unsigned short", "unsigned short int", "int",
              "signed", "signed int", "unsigned", "unsigned int", "long", "long int",
              "signed long", "signed long int", "unsigned long", "unsigned long int", "wchar_t"}


def enum_names():
    """The enum type names in game/include: `enum Name {` and `typedef enum {...} Name;`. (An enum
    parameter is one word; a class or struct by value is not, so the thunks can't forward it.)"""
    names = set()
    for path in glob.glob(os.path.join(ROOT, "game", "include", "**", "*.h"), recursive=True):
        text = open(path, encoding="utf-8", errors="replace").read()
        names.update(re.findall(r"\benum\s+(\w+)\s*[{:]", text))
        names.update(re.findall(r"\btypedef\s+enum\b[^{;]*\{[^}]*\}\s*(\w+)\s*;", text))
        names.update(re.findall(r"\btypedef\s+enum\s+\w+\s+(\w+)\s*;", text))
    return names


def check_params(sig, params, enums):
    """Fail the build when a parameter of the demangled signature sig isn't one word."""
    for p in params:
        if "*" in p or "&" in p:
            continue  # pointers and references
        toks = [t for t in p.split() if t not in ("const", "volatile")]
        key = " ".join(toks)
        if key in WORD_TYPES or key.split("::")[-1] in enums:
            continue
        if "float" in toks or "double" in toks:
            what = "floating point"
        elif toks.count("long") >= 2:
            what = "a 64-bit integer"
        else:
            what = "a class or struct by value (or a type not known to be one word)"
        raise SystemExit("link: %s: parameter '%s' is %s; the thunks forward each parameter as one "
                         "word (pass a pointer or a reference)" % (sig, p, what))


def thunks_c(metas, unowned, enums):
    """link()'s thunks.c: the virtual-dispatch thunks and the destructor and delete adapters, in
    plain C. Each forwards its parameters by name, as intptr_t words (check_params has rejected the
    rest). Returns (the C source, the C names of the functions it defines).

    - pcvt_<key>(this, args): calls the retail vtable's entry (slot + 1) with this + delta.
      Dtor thunks (`~`) first compute in_chrg from the original this (delete_flag).
    - pcdtorcall_<a>(this): base-object destructor call (in_chrg 0); pcdtorcalld_<a>: the
      complete-object one (in_chrg from delete_flag).
    - pcdel_<a>(p): operator delete for a delete statement, skipped for the object its destructor
      already freed; pc_delete_unowned when a is libsn's (unowned).
    - pcfn_<a>: a constructor (the native one, then `this`), or a destructor (the native base one,
      then the delete when in_chrg bit 0 is set)."""
    ext = {}     # external name -> its declaration (psx_<addr> etc. are bound by psx_syms.ld)
    body = []
    emitted = set()
    arity = {}   # thunk key -> its number of parameters (one definition per key)
    done = set()

    def psx(a):
        ext["psx_%08x" % a] = "extern void psx_%08x();" % a
        return "psx_%08x" % a

    def callee(a):  # the function a delete of address a runs
        if a == unowned:
            ext["pc_delete_unowned"] = "extern void pc_delete_unowned();"
            return "pc_delete_unowned"
        return psx(a)

    def delete_flag():  # in_chrg: 3 for the object a delete statement marked, else 0
        return ["if (self == pc_delete_marked) { pc_delete_freed = self; pc_delete_marked = 0;"
                " in_chrg = 3; }",
                "else in_chrg = 0;"]

    def define(cname, lines):
        body.extend(lines)
        emitted.add(cname)

    for m in metas:
        for t in m.get("thunks", []):
            key = t["key"]
            name = "pcvt_" + key
            n = 0 if t["dtor"] else len(t["params"])
            if key in arity:
                if arity[key] != n:
                    raise SystemExit("link: thunk %s is called with %d and %d parameters (%s)"
                                     % (key, arity[key], n, t["sig"]))
                continue
            arity[key] = n
            ext[t["off"]] = "extern int %s;" % t["off"]
            entry = "PsxVtEntry *e = (PsxVtEntry *)*(void **)((char *)self + %s) + %d;" % (
                t["off"], t["slot"] + 1)
            if t["dtor"]:
                define(name, ["void %s(void *self, intptr_t in_chrg) {" % name,
                              "    " + entry] + ["    " + ln for ln in delete_flag()] + [
                    "    ((void (*)(void *, intptr_t))e->pfn)((char *)self + e->delta, in_chrg);",
                    "}"])
            else:
                check_params(t["sig"], t["params"], enums)
                ps = ["a%d" % (i + 1) for i in range(n)]
                define(name, [
                    "uint64_t %s(%s) {" % (name, ", ".join(["void *self"] +
                                                           ["intptr_t " + p for p in ps])),
                    "    " + entry,
                    "    return ((uint64_t (*)(%s))e->pfn)(%s);" % (
                        ", ".join(["void *"] + ["intptr_t"] * n),
                        ", ".join(["(char *)self + e->delta"] + ps)),
                    "}"])
        for a in m.get("dtor_calls", []):
            if ("d", a) not in done:
                done.add(("d", a))
                define("pcdtorcall_%08x" % a, [
                    "void pcdtorcall_%08x(void *self) { %s(self, 0); }" % (a, psx(a))])
        for a in m.get("dtor_calls_d", []):
            if ("dd", a) not in done:
                done.add(("dd", a))
                define("pcdtorcalld_%08x" % a, ["void pcdtorcalld_%08x(void *self) {" % a,
                                                "    intptr_t in_chrg;"] +
                       ["    " + ln for ln in delete_flag()] +
                       ["    %s(self, in_chrg);" % psx(a), "}"])
        for a in m.get("del_wrappers", []):
            if ("del", a) not in done:
                done.add(("del", a))
                define("pcdel_%08x" % a, [
                    "void pcdel_%08x(void *p) {" % a,
                    "    if (p == pc_delete_freed) { pc_delete_freed = 0; return; }",
                    "    pc_delete_marked = 0;",
                    "    %s(p);" % callee(a),
                    "}"])
    # constructors defined in C++: the native one with the same parameters, then `this` (egcs)
    for m in metas:
        for a in m.get("ctor_defs", []):
            if ("ctor", a) in done:
                continue
            done.add(("ctor", a))
            info = m.get("ctor_sigs", {}).get(str(a))
            if info is None:
                raise SystemExit("link: constructor 0x%08x has no signature (rebuild the TUs)" % a)
            check_params(info["sig"], info["params"], enums)
            ps = ["a%d" % (i + 1) for i in range(len(info["params"]))]
            ext["pcctor_%08x" % a] = "extern void pcctor_%08x();" % a
            define("pcfn_%08x" % a, [
                "void *pcfn_%08x(%s) {" % (a, ", ".join(["void *self"] + ["intptr_t " + p for p in ps])),
                "    pcctor_%08x(%s);" % (a, ", ".join(["self"] + ps)),
                "    return self;",
                "}"])
    # destructors defined in C++: the egcs one is (this, in_chrg): the native base-object
    # destructor, then the delete when in_chrg bit 0 is set
    for m in metas:
        for a, cls, dl in m.get("dtor_defs", []):
            if ("dtor", a) in done:
                continue
            done.add(("dtor", a))
            if not dl:
                raise SystemExit("no operator delete for %s (destructor 0x%08x)" % (cls, a))
            ext["pcdtor_%08x" % a] = "extern void pcdtor_%08x();" % a
            define("pcfn_%08x" % a, [
                "void pcfn_%08x(void *self, intptr_t in_chrg) {" % a,
                "    pcdtor_%08x(self);" % a,
                "    if (in_chrg & 1) %s(self);" % callee(dl),
                "}"])
    head = ["/* generated by link.py (thunks_c) */",
            "#include <stdint.h>",
            "",
            "/* an egcs vtable entry (game/data/psxdata.h): {delta, index, function}, after an",
            "   all-zero entry; slot k is entry k + 1 */",
            "typedef struct { short delta; short index; void *pfn; } PsxVtEntry;",
            "extern void *pc_delete_marked, *pc_delete_freed;"]
    head += [ext[k] for k in sorted(ext)]
    return "\n".join(head + [""] + body) + "\n", emitted


def psx_symbols(syms, game, stubs, gen, aliases=()):
    """link()'s psx_syms.ld: every `psx_<addr>` the code or the data names is a function, its
    native code (pcfn_<addr>) or its stub. (As PE spells the names: `_psx_80012345`, and
    `_psx_80012345_a1` for a second name objcopy needed.) `aliases`: (name, name it equals), for
    the thunks' second names."""
    ld = []
    stub_set = set(stubs)
    for s in sorted(syms):
        a = int(s[5:13], 16)
        name = s if USCORE else s[1:]
        if a in game or a in stub_set:
            ld.append("%s = %s%s_%08x;\n" % (name, USCORE, "pcfn" if a in game else "pcstub", a))
        else:
            raise SystemExit("link: %s is no function (data is linked by name)" % s)
    ld += ["%s = %s;\n" % (al, base) for al, base in aliases]
    write_if_changed(os.path.join(gen, "psx_syms.ld"), "".join(ld))


def data_names(ctx, gen):
    """link()'s data_names.c: PC_DataByName, the game's data by C name (THPS_POKE)."""
    names = sorted(n for n in ctx.data_defined if re.fullmatch(r"[A-Za-z_]\w*", n))
    lines = ["#include <string.h>", '#include "pc_runtime.h"']
    lines += ["extern char %s[];" % n for n in names]
    lines.append("static const struct { const char *n; char *p; } T[] = {")
    lines += ['\t{"%s", %s},' % (n, n) for n in names]
    lines.append("};")
    lines.append("void *PC_DataByName(const char *n) { int lo = 0, hi = (int)(sizeof T / sizeof T[0])"
                 " - 1; while (lo <= hi) { int m = (lo + hi) / 2, c = strcmp(T[m].n, n);"
                 " if (!c) return T[m].p; if (c < 0) lo = m + 1; else hi = m - 1; } return NULL; }")
    write_if_changed(os.path.join(gen, "data_names.c"), "\n".join(lines) + "\n")


def runtime_flags(toolbin):
    """(compile flags, SDL link flags) for the runtime (platform/pc/src); C adds -std=gnu11, C++
    CXX_RT_FLAGS."""
    if WINDOWS:
        sdl_cflags = ["-I" + os.path.join(os.path.dirname(toolbin), "include", "SDL2"),
                      "-I" + os.path.join(os.path.dirname(toolbin), "include")]
        sdl_libs = []
    else:
        sdl_cflags, sdl_libs = sdl_flags()
    rt_flags = SANITIZE + [MFLAG, "-O2", "-g", "-fno-strict-aliasing", "-fwrapv",
                "-fno-omit-frame-pointer", "-Wall", "-Wno-unused-function",
                "-Wno-unused-variable", "-I" + os.path.join(PC, "include"),
                "-I" + os.path.join(PC, "include", "psyq")] + sdl_cflags
    if not WINDOWS:
        # absolute addresses (no PIE), the Windows ABI's small-struct returns (as the game code,
        # build.py), 64-bit file offsets (disc images are bigger than 2 GB)
        rt_flags += ["-fno-pic", "-fno-pie", "-D_FILE_OFFSET_BITS=64"] + \
            ([] if X64 else ["-freg-struct-return"])
    return rt_flags, sdl_libs


def link(ctx, metas, out, toolbin, release=False, version=None):
    gcc = tool("gcc")
    gxx = tool("g++")
    objroot = out
    if release:
        out = os.path.join(out, "release")
    nm = tool("nm")
    gen = os.path.join(out, "gen")
    rt = os.path.join(out, "runtime")
    os.makedirs(gen, exist_ok=True)
    os.makedirs(rt, exist_ok=True)
    rt_flags, sdl_libs = runtime_flags(toolbin)

    # ---- 1. runtime -------------------------------------------------------------------------
    # the runtime is C++ (glad's generated loader stays C)
    srcs = sorted(glob.glob(os.path.join(PC, "src", "*.c")) +
                  glob.glob(os.path.join(PC, "src", "*.cpp")))
    hdrs = glob.glob(os.path.join(PC, "include", "**", "*.h"), recursive=True)

    def cc(src, obj, flags):
        if os.path.exists(obj) and os.path.getmtime(obj) >= os.path.getmtime(src) and \
                all(os.path.getmtime(obj) >= os.path.getmtime(h) for h in hdrs):
            return obj
        if src.endswith(".cpp"):
            run([gxx] + flags + CXX_RT_FLAGS + ["-c", src, "-o", obj])
        else:
            run([gcc] + flags + ["-std=gnu11", "-Werror=implicit-function-declaration", "-c", src,
                 "-o", obj])
        return obj

    with cf.ThreadPoolExecutor() as ex:
        rt_objs = list(ex.map(lambda s: cc(s, os.path.join(rt, os.path.splitext(
            os.path.basename(s))[0] + ".o"), rt_flags), srcs))
    defined = set()
    for o in rt_objs:
        for line in run([nm, "-g", "--defined-only", o]).splitlines():
            p = line.split()
            if len(p) == 3 and p[1] == "T":
                defined.add(p[2][len(USCORE):] if p[2].startswith(USCORE) else p[2])

    # ---- 2. which addresses have which implementation ----------------------------------------
    psyq = {}
    for line in open(os.path.join(ROOT, "game", "psyq_names.tsv")):
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
    lib_entries = set(ctx.lib_calls)  # the retail code's (game/retail_refs.tsv)
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
    # ---- 4. tables, stubs, thunks, names ----------------------------------------------------
    # every code address the game's code or data names: a function, else a stub (library tables
    # in the data point at library functions the port doesn't have)
    data_psx = ctx.data["psx_syms"]
    referenced = {int(s[5:13], 16) for s in data_psx}
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
    # libsn's operator delete freed into the BIOS heap, which the game never allocates from: on
    # PC it is the native pc_delete_unowned (build.py NATIVE_RENAME)
    unowned = psyq.get("__builtin_delete", [None])[0]
    # Thunks (thunks_c): the virtual dispatch, the destructor-call and delete adapters, and the
    # constructor and destructor wrappers, in C
    thunk_src, thunk_names = thunks_c(metas, unowned, enum_names())
    write_if_changed(os.path.join(gen, "thunks.c"), thunk_src)
    # extra names objcopy needed when two symbols in one object map to the same thunk: psx_syms.ld
    # gives each the name it stands for
    aliases = set()
    for m in metas:
        aliases.update(m.get("aliases", []))
    alias_ld = []
    for al in sorted(aliases):
        base = re.sub(r"_a\d+$", "", al)
        if (base[1:] if USCORE else base) in thunk_names:
            alias_ld.append((al, base))

    # psx_<addr>: the functions the game's code and data refer to by retail address, each its
    # native code (or stub). The data itself is linked by name (game/data's objects).
    syms = set(data_psx)
    syms.update("_psx_%08x" % a for a in list(game) + stubs)
    for m in metas:
        syms.update(m.get("psx_syms", []))
    psx_symbols(syms, game, stubs, gen, alias_ld)
    data_names(ctx, gen)
    # the build's version (PC_Version, pc_runtime.h)
    ver, ver_full, ver_nums = build_version(version)
    write_if_changed(os.path.join(gen, "version.c"),
                     "/* generated by link.py: build.py --version */\n"
                     "const char PC_Version[] = \"%s\";\n" % ver_full)

    # ---- 5. compile generated sources and link ------------------------------------------------
    gen_objs = []
    gflags = rt_flags + ["-O1", "-Wno-unused-label", "-Wno-unused-but-set-variable",
                         "-Wno-maybe-uninitialized"]
    jobs = [(os.path.join(gen, n), os.path.join(gen, n[:-2] + ".o"))
            for n in ["thunks.c", "stubs.c", "names.c", "data_names.c", "version.c"]]
    with cf.ThreadPoolExecutor() as ex:
        gen_objs = list(ex.map(lambda j: cc(j[0], j[1], gflags), jobs))
    gen_objs += ctx.data["objs"]


    if WINDOWS:
        # application icon: SDL2 gives the window the exe's first icon, so this also sets the
        # taskbar and title-bar icon. And the version resource
        ico = os.path.join(ROOT, "icon.ico")
        write_if_changed(os.path.join(gen, "thps.rc"),
                         '1 ICON "%s"\n' % ico.replace("\\", "/") +
                         version_rc(ver, ver_full, ver_nums))
        res_o = os.path.join(gen, "thps_res.o")
        if not os.path.exists(res_o) or os.path.getmtime(res_o) < max(
                os.path.getmtime(ico), os.path.getmtime(os.path.join(gen, "thps.rc"))):
            run([tool("windres"), "-F", "pe-x86-64" if X64 else "pe-i386",
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
        ldflags = ["-mwindows" if release else "-mconsole"]
        if not X64:
            ldflags += ["-Wl,--large-address-aware", "-Wl,--disable-dynamicbase",
                        "-Wl,--image-base=0x400000"]
        elif not release:
            # a fixed base above 4 GB: stable addresses for symbolize.py, and a pointer cut to 32
            # bits faults
            ldflags += ["-Wl,--disable-dynamicbase", "-Wl,--image-base=0x140000000"]
    else:
        # SDL2 and the C library are the system's; OpenGL comes through SDL. Not position
        # independent: the image sits at its default 0x08048000
        libs = sdl_libs + ["-lm", "-lpthread"]
        # the runtime's C++ library is linked in (no libstdc++ version to match on the player's system)
        ldflags = ["-no-pie", "-Wl,-z,noexecstack", "-static-libstdc++", "-static-libgcc"]
    run([gxx, MFLAG, "-g", "@" + rsp, "-o", exe] + SANITIZE + ldflags +
        ["-Wl," + os.path.join(gen, "psx_syms.ld").replace("\\", "/")] + libs)
    if release:
        # the debug info goes to thps.debug (for symbolize.py; not shipped)
        dbg = os.path.join(out, "thps.debug")
        run([tool("objcopy"), "--only-keep-debug", exe, dbg])
        run([tool("strip"), "--strip-all", exe])
        run([tool("objcopy"), "--add-gnu-debuglink=thps.debug", exe], cwd=out)
        package(out, toolbin, version)
    print("linked %s (%s): %d game functions, %d stubs" % (exe, ver_full, len(game), len(stubs)))
    return 0
