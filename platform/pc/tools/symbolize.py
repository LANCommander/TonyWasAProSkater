"""Turn crash-log addresses into function/file:line: python symbolize.py ADDR... (or stdin)."""
import os, re, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, "..", "..", "..", "build", "pc", "thps.exe")
ADDR2LINE = os.path.join(os.environ.get("THPS_MINGW", r"C:\msys64\mingw32\bin"), "addr2line.exe")
addrs = sys.argv[1:] or re.findall(r"\b[0-9a-fA-F]{8}\b", sys.stdin.read())
out = subprocess.run([ADDR2LINE, "-f", "-C", "-p", "-e", EXE] + addrs, capture_output=True, text=True).stdout
for a, line in zip(addrs, out.splitlines()):
    print(a, line)
