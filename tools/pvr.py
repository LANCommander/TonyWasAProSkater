"""Decode Dreamcast PowerVR texture payloads (the texture records in v6 .PSX files and .PVR files).

pixel formats: 0 ARGB1555, 1 RGB565, 2 ARGB4444
data formats: 1 twiddled, 2 twiddled+mips, 3 VQ, 4 VQ+mips, 5/6 pal4 twiddled, 7/8 pal8, 9 rectangle,
              0x0d rectangle twiddled, 0x10/0x11 small VQ (+mips), 0x12 twiddled mips (alt)
Mip chains are stored smallest first; the full-size level is the last one (twiddled chains carry
padding in front, VQ chains at the end). platform/pc/src/pc_dctex.c is the port's C version.
"""
import struct


def untwiddle_index(x, y):
    # Morton order with the y bit in the low position
    i = 0
    b = 0
    while (1 << b) <= max(x, y):
        i |= ((y >> b) & 1) << (2 * b)
        i |= ((x >> b) & 1) << (2 * b + 1)
        b += 1
    return i


def to_rgba(p, fmt):
    if fmt == 0:
        return ((p >> 10 & 31) * 255 // 31, (p >> 5 & 31) * 255 // 31, (p & 31) * 255 // 31,
                255 if p & 0x8000 else 0)
    if fmt == 1:
        return ((p >> 11 & 31) * 255 // 31, (p >> 5 & 63) * 255 // 63, (p & 31) * 255 // 31, 255)
    if fmt == 2:
        return ((p >> 8 & 15) * 17, (p >> 4 & 15) * 17, (p & 15) * 17, (p >> 12 & 15) * 17)
    raise ValueError("pixel format %d" % fmt)


def _twiddled(data, off, w, h, fmt):
    # non-square twiddled textures are tiled squares of min(w, h)
    s = min(w, h)
    out = [None] * (w * h)
    words = struct.unpack_from("<%dH" % (w * h), data, off)
    for y in range(h):
        for x in range(w):
            tile = (x // s) + (y // s) * (w // s) if w > h else (y // s)
            idx = tile * s * s + untwiddle_index(x % s, y % s)
            out[y * w + x] = to_rgba(words[idx], fmt)
    return out


def _mip_skip(w, bpp_num, bpp_den, vq=False):
    # bytes of all mip levels smaller than w (square), smallest (1x1) first
    total = 0
    m = 1
    while m < w:
        if vq:
            total += max(1, m * m // 4)  # 1x1 level occupies one index
        else:
            total += m * m * bpp_num // bpp_den
        m *= 2
    return total


def decode(data, off, w, h, pfmt, dfmt, size=None):
    """Return a list of w*h RGBA tuples for the full-size level. size = payload bytes; when given,
    the full-size level is located from the end (mip chains carry a little padding)."""
    if dfmt in (1, 2, 0x0d, 0x12):
        if dfmt in (2, 0x12):
            off = off + size - w * h * 2 if size else off + _mip_skip(w, 2, 1) + 2
        return _twiddled(data, off, w, h, pfmt)
    if dfmt in (3, 4):
        book = [to_rgba(p, pfmt) for p in struct.unpack_from("<1024H", data, off)]
        off += 2048
        if dfmt == 4:
            off += _mip_skip(w, 0, 1, vq=True)  # VQ chains pad at the end, not the start
        out = [None] * (w * h)
        hw, hh = w // 2, h // 2
        for y in range(hh):
            for x in range(hw):
                code = data[off + untwiddle_index(x, y)]
                # each code is a 2x2 block stored twiddled: (0,0) (0,1) (1,0) (1,1)
                b = code * 4
                out[(2 * y) * w + 2 * x] = book[b]
                out[(2 * y + 1) * w + 2 * x] = book[b + 1]
                out[(2 * y) * w + 2 * x + 1] = book[b + 2]
                out[(2 * y + 1) * w + 2 * x + 1] = book[b + 3]
        return out
    if dfmt == 9:
        words = struct.unpack_from("<%dH" % (w * h), data, off)
        return [to_rgba(p, pfmt) for p in words]
    raise ValueError("data format %#x" % dfmt)
