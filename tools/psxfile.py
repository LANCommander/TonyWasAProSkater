"""Parse a Neversoft .PSX model/level file (THPS1 PS1 = version 4, Dreamcast = version 6).

Layout as ProcessNewPSX / Spool_LoadTexturesToVRAM read it:
  u32 version, u32 packets_offset, u32 num_objects, objects[num_objects] (36 bytes),
  u32 num_models, u32 model_offsets[num_models], ...models...,
  packets @ packets_offset: {u32 type, u32 size, size bytes}... u32 -1,
  u32 model_checksums[num_models], u32 num_tex_checksums, u32 tex_checksums[],
  u32 n16, {u32 checksum, u16 clut[16]}[n16], u32 n256, {u32 checksum, u16 clut[256]}[n256],
  u32 num_textures, u32 texture_offsets[], texture records
  {u32 flags, u32 num_colours, u32 palette_checksum, u32 checksum_index, u16 w, u16 h, data}.

usage: psxfile.py <file> [-v]
"""
import struct
import sys


class PSXFile:
    def __init__(self, data):
        self.data = data
        u32 = lambda o: struct.unpack_from("<I", data, o)[0]
        self.u32 = u32
        self.version, self.packets_off, self.num_objects = struct.unpack_from("<3I", data, 0)
        mo = 12 + self.num_objects * 36
        self.num_models = u32(mo)
        self.model_offsets = list(struct.unpack_from("<%dI" % self.num_models, data, mo + 4))
        self.packets = []
        q = self.packets_off
        while u32(q) != 0xFFFFFFFF:
            t, s = u32(q), u32(q + 4)
            self.packets.append((t, q + 8, s))
            q += 8 + s
        q += 4
        self.model_checksums = list(struct.unpack_from("<%dI" % self.num_models, data, q))
        q += 4 * self.num_models
        n = u32(q)
        self.tex_checksums = list(struct.unpack_from("<%dI" % n, data, q + 4))
        q += 4 + 4 * n
        self.pal16 = []
        n = u32(q); q += 4
        for _ in range(n):
            self.pal16.append((u32(q), data[q + 4:q + 36])); q += 36
        self.pal256 = []
        n = u32(q); q += 4
        for _ in range(n):
            self.pal256.append((u32(q), data[q + 4:q + 516])); q += 516
        self.tex_table_off = q
        n = u32(q); q += 4
        self.textures = []
        offs = struct.unpack_from("<%dI" % n, data, q) if n < 100000 else []
        for o in offs:
            flags, ncol, pal, idx = struct.unpack_from("<4I", data, o)
            w, h = struct.unpack_from("<2H", data, o + 16)
            self.textures.append(dict(off=o, flags=flags, ncol=ncol, pal=pal, idx=idx, w=w, h=h,
                                      checksum=self.tex_checksums[idx] if idx < len(self.tex_checksums) else None))

    def models(self):
        for off in self.model_offsets:
            flags, nv, nn, nf = struct.unpack_from("<4H", self.data, off)
            yield off, flags, nv, nn, nf


def main():
    f = PSXFile(open(sys.argv[1], "rb").read())
    print("version %d  size %d  objects %d  models %d  packets %s" % (
        f.version, len(f.data), f.num_objects, f.num_models,
        ["%x:%d" % (t, s) for t, _, s in f.packets]))
    print("tex checksums %d  pal16 %d  pal256 %d  textures %d  (table @%x)" % (
        len(f.tex_checksums), len(f.pal16), len(f.pal256), len(f.textures), f.tex_table_off))
    if "-v" in sys.argv:
        for t in f.textures:
            print("  %08x %3dx%-3d ncol %d flags %x pal %08x @%x" % (
                t["checksum"] or 0, t["w"], t["h"], t["ncol"], t["flags"], t["pal"], t["off"]))


if __name__ == "__main__":
    main()
