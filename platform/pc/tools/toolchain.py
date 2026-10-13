"""The host toolchain: MinGW-w64 i686 on Windows (PE/COFF), the system's multilib GCC elsewhere
(Linux: ELF, `gcc -m32`).

THPS_MINGW: the MinGW bin folder (Windows; default C:\\msys64\\mingw32\\bin).
THPS_TOOLBIN: a folder with gcc, g++, as, nm, objcopy... (other systems; default: PATH).
THPS_BUILD_DIR: where builds go (default build/pc on Windows, build/pc-linux elsewhere, so both
can be built from one checkout).
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))

WINDOWS = sys.platform == "win32"
if WINDOWS:
    TOOLBIN = os.environ.get("THPS_MINGW", r"C:\msys64\mingw32\bin")
else:
    TOOLBIN = os.environ.get("THPS_TOOLBIN", "")
EXE_SUFFIX = ".exe" if WINDOWS else ""
EXE_NAME = "thps" + EXE_SUFFIX
OUT = os.environ.get("THPS_BUILD_DIR") or os.path.join(ROOT, "build", "pc" if WINDOWS else "pc-linux")
# assembler names of C symbols: PE prefixes them with '_', ELF doesn't
USCORE = "_" if WINDOWS else ""


def tool(name):
    """A toolchain program: gcc, g++, as, nm, objcopy, c++filt, addr2line, objdump..."""
    if WINDOWS:
        return os.path.join(TOOLBIN, name + ".exe")
    return os.path.join(TOOLBIN, name) if TOOLBIN else name


def add_to_path():
    if TOOLBIN:
        os.environ["PATH"] = TOOLBIN + os.pathsep + os.environ["PATH"]
