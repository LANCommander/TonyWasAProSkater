"""List / extract the ISO9660 filesystem of a Dreamcast GD-ROM dump (.gdi + track files).

The high-density area starts at LBA 45000; the filesystem's PVD lives in the first data track
there and file extents use absolute LBAs that may land in any later data track.

usage: gdi_extract.py <file.gdi> [outdir]     (no outdir = list only)
"""
import os
import struct
import sys


def load_tracks(gdi):
    base = os.path.dirname(gdi)
    lines = open(gdi).read().split("\n")
    tracks = []
    for ln in lines[1:]:
        p = ln.split()
        if len(p) < 6:
            continue
        num, lba, ctype, ssize = int(p[0]), int(p[1]), int(p[2]), int(p[3])
        name = " ".join(p[4:-1]).strip('"')
        tracks.append(dict(num=num, lba=lba, data=ctype == 4, ssize=ssize,
                           path=os.path.join(base, name)))
    for t in tracks:
        t["count"] = os.path.getsize(t["path"]) // t["ssize"]
        t["f"] = open(t["path"], "rb")
    return tracks


class Disc:
    def __init__(self, gdi):
        self.tracks = load_tracks(gdi)

    def sector(self, lba):
        for t in self.tracks:
            if t["data"] and t["lba"] <= lba < t["lba"] + t["count"]:
                off = 16 if t["ssize"] == 2352 else 0
                t["f"].seek((lba - t["lba"]) * t["ssize"] + off)
                return t["f"].read(2048)
        raise ValueError("lba %d not in a data track" % lba)

    def read(self, lba, size):
        out = bytearray()
        while len(out) < size:
            out += self.sector(lba)
            lba += 1
        return bytes(out[:size])


def walk(disc, lba, size, path, out):
    data = disc.read(lba, size)
    pos = 0
    while pos < len(data):
        ln = data[pos]
        if ln == 0:
            pos = (pos // 2048 + 1) * 2048
            continue
        rec = data[pos:pos + ln]
        ext = struct.unpack_from("<I", rec, 2)[0]
        sz = struct.unpack_from("<I", rec, 10)[0]
        flags = rec[25]
        nl = rec[32]
        name = rec[33:33 + nl]
        pos += ln
        if name in (b"\0", b"\1"):
            continue
        name = name.decode("ascii", "replace").split(";")[0]
        full = path + "/" + name
        if flags & 2:
            walk(disc, ext, sz, full, out)
        else:
            out.append((full, ext, sz))


def main():
    disc = Disc(sys.argv[1])
    hd = [t for t in disc.tracks if t["data"] and t["lba"] >= 45000][0]
    pvd = disc.sector(hd["lba"] + 16)
    assert pvd[1:6] == b"CD001", pvd[:8]
    root = pvd[156:156 + 34]
    files = []
    walk(disc, struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0], "", files)
    outdir = sys.argv[2] if len(sys.argv) > 2 else None
    for name, lba, sz in files:
        print("%8d %10d %s" % (lba, sz, name))
        if outdir:
            dst = os.path.join(outdir, name.lstrip("/"))
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            with open(dst, "wb") as f:
                f.write(disc.read(lba, sz))
    print("%d files, %d bytes" % (len(files), sum(f[2] for f in files)))


if __name__ == "__main__":
    main()
