"""The host toolchain: MinGW-w64 on Windows (PE/COFF), the system's GCC elsewhere (Linux: ELF).

THPS_ARCH: x86 (32-bit, the default) or x64 (64-bit).
THPS_MINGW: the MinGW bin folder (Windows; default C:\\msys64\\mingw32\\bin, x64:
C:\\msys64\\ucrt64\\bin). THPS_MINGW32: the 32-bit one, which x64 builds use too (data_image.py).
THPS_TOOLBIN: a folder with gcc, g++, as, nm, objcopy... (other systems; default: PATH).
THPS_BUILD_DIR: where builds go (default build/pc on Windows, build/pc-linux elsewhere, with a
`64` suffix for x64, so every combination can be built from one checkout).
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))

WINDOWS = sys.platform == "win32"
ARCH = os.environ.get("THPS_ARCH", "x86")
if ARCH not in ("x86", "x64"):
    raise SystemExit("THPS_ARCH must be x86 or x64")
X64 = ARCH == "x64"
if WINDOWS:
    # the 32-bit toolchain too: the x64 build checks the decompiled data in its retail (32-bit)
    # layout (data_image.py)
    TOOLBIN32 = os.environ.get("THPS_MINGW32", r"C:\msys64\mingw32\bin")
    TOOLBIN64 = os.environ.get("THPS_MINGW64", r"C:\msys64\ucrt64\bin")
    TOOLBIN = os.environ.get("THPS_MINGW", TOOLBIN64 if X64 else TOOLBIN32)
else:
    TOOLBIN = TOOLBIN32 = TOOLBIN64 = os.environ.get("THPS_TOOLBIN", "")
EXE_SUFFIX = ".exe" if WINDOWS else ""
EXE_NAME = "thps" + EXE_SUFFIX
OUT = os.environ.get("THPS_BUILD_DIR") or os.path.join(
    ROOT, "build", ("pc" if WINDOWS else "pc-linux").replace("pc", "pc64" if X64 else "pc", 1))
# assembler names of C symbols: 32-bit PE prefixes them with '_', x64 PE and ELF don't
USCORE = "_" if WINDOWS and not X64 else ""
# the compiler flag for the word size
MFLAG = "-m64" if X64 else "-m32"
PTR = 8 if X64 else 4


def tool(name, bits=None):
    """A toolchain program: gcc, g++, as, nm, objcopy, c++filt, addr2line, objdump... (bits=32:
    the 32-bit toolchain's, whatever THPS_ARCH is)"""
    d = {32: TOOLBIN32, 64: TOOLBIN64}.get(bits, TOOLBIN)
    if WINDOWS:
        return os.path.join(d, name + ".exe")
    return os.path.join(d, name) if d else name


def add_to_path():
    if TOOLBIN:
        os.environ["PATH"] = TOOLBIN + os.pathsep + os.environ["PATH"]


def env(bits=None):
    """The environment to run tool(name, bits) in: its folder first on PATH (MinGW's compilers
    find their DLLs there)."""
    d = {32: TOOLBIN32, 64: TOOLBIN64}.get(bits, TOOLBIN)
    e = dict(os.environ)
    if d:
        e["PATH"] = d + os.pathsep + e["PATH"]
    return e
