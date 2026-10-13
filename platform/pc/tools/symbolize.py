"""Turn crash-log addresses into function/file:line: python symbolize.py ADDR... (or stdin).
THPS_SYMBOLS: the exe or debug file to read (default build/pc/thps.exe, on Linux
build/pc-linux/thps; for a release build's thps.log, that build's release/thps.debug)."""
import os, re, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import toolchain  # noqa: E402
EXE = os.environ.get("THPS_SYMBOLS") or os.path.join(toolchain.OUT, toolchain.EXE_NAME)
ADDR2LINE = toolchain.tool("addr2line")
addrs = sys.argv[1:] or re.findall(r"\b[0-9a-fA-F]{8}\b", sys.stdin.read())
out = subprocess.run([ADDR2LINE, "-f", "-C", "-p", "-e", EXE] + addrs, capture_output=True, text=True).stdout
for a, line in zip(addrs, out.splitlines()):
    print(a, line)
