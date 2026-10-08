"""Name the Psy-Q library functions linked into retail THPS1.

Scans the retail executable's library region with the masked byte signatures that ship with
ghidra_psx_ldr (one per library object, per SDK version) and writes every label of every object
that matches exactly once: build/psyq_names.tsv (addr, name, lib, obj, sdk version).

usage: python tools/psyq_sigmatch.py [--exe extern/thps1_retail/SLUS_008.60]
"""
import argparse, glob, json, os, re, struct

SIGDIR = r"D:\Tools\ghidra\Ghidra\Extensions\ghidra_psx_ldr\data\psyq"
LIB_START = 0x800858AC  # first byte after the game code (see decomp/functions.tsv)


def load_exe(path):
    d = open(path, "rb").read()
    taddr, tsize = struct.unpack_from("<2I", d, 0x18)
    return taddr, d[0x800:0x800 + tsize]


def compile_sig(sig):
    toks = sig.split()
    pat = b"".join(b"." if t == "??" else re.escape(bytes([int(t, 16)])) for t in toks)
    return re.compile(pat, re.S), len(toks)


def is_func_label(name):
    return not name.startswith(("loc_", "text_", "data_", "bss_"))


def match_object(o, img, lo):
    """Yield (offset, labels, siglen): the whole object if it matches once, else each function
    (the span from one named label to the next) that matches once on its own."""
    toks = o["sig"].split()
    rx, n = compile_sig(o["sig"])
    hits = [h for h in (m.start() for m in rx.finditer(img, lo)) if h % 4 == 0]
    labs = [l for l in o["labels"] if is_func_label(l["name"])]
    if len(hits) == 1:
        yield hits[0], labs, n
        return
    offs = sorted({l["offset"] for l in labs})
    for i, off in enumerate(offs):
        end = offs[i + 1] if i + 1 < len(offs) else len(toks)
        chunk = toks[off:end]
        if len(chunk) < 24 or sum(t != "??" for t in chunk) < 16:
            continue
        crx, cn = compile_sig(" ".join(chunk))
        ch = [h for h in (m.start() for m in crx.finditer(img, lo)) if h % 4 == 0]
        if len(ch) == 1:
            yield ch[0] - off, [l for l in labs if l["offset"] == off], cn


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default="extern/thps1_retail/SLUS_008.60")
    ap.add_argument("--out", default="build/psyq_names.tsv")
    a = ap.parse_args()
    base, img = load_exe(a.exe)
    lo = LIB_START - base
    found = {}  # addr -> (name, lib, obj, ver, siglen)
    for ver in sorted(os.listdir(SIGDIR)):
        vdir = os.path.join(SIGDIR, ver)
        if not os.path.isdir(vdir):
            continue
        for jf in glob.glob(os.path.join(vdir, "*.json")):
            lib = os.path.basename(jf)[:-5]
            try:
                objs = json.load(open(jf))
            except Exception:
                continue
            for o in objs:
                if not o.get("sig"):
                    continue
                for at, labs, n in match_object(o, img, lo):
                    for lab in labs:
                        name = lab["name"]
                        addr = base + at + lab["offset"]
                        prev = found.get(addr)
                        # prefer the longest (most specific) signature, then the newest SDK
                        if prev is None or (n, ver) > (prev[4], prev[3]):
                            found[addr] = (name, lib, o["name"], ver, n)
    os.makedirs(os.path.dirname(a.out), exist_ok=True)
    with open(a.out, "w") as f:
        f.write("# addr\tname\tlib\tobj\tsdk\n")
        for addr in sorted(found):
            name, lib, obj, ver, _ = found[addr]
            f.write(f"0x{addr:08x}\t{name}\t{lib}\t{obj}\t{ver}\n")
    print(f"{len(found)} labels -> {a.out}")


if __name__ == "__main__":
    main()
