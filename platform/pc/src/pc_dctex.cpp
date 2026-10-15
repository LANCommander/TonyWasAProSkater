/* Dreamcast models and textures: version 6 .psx files and their PowerVR textures (see
 * pc_dctex.h).
 *
 * File layout (as ProcessNewPSX and Spool_LoadTexturesToVRAM read it): version, offset of the
 * packets, objects (36 bytes each), the model table and models, the packets (type, size, data;
 * -1 ends them), one checksum per model, the texture checksum table, the 16- and 256-colour
 * palettes (none in version 6), then the texture table: a count, an offset per texture, and
 * the records {flags, colours, palette checksum, checksum index, u16 w, u16 h, data}. In
 * version 6 a record has 65536 colours and its data is a PowerVR image: u8 pixel format,
 * u8 data format, u16 0, u32 size (of what follows, + 8), the image.
 *
 * PowerVR images: pixels are ARGB1555 (0), RGB565 (1) or ARGB4444 (2). Twiddled data (1, and 2
 * with mipmaps) stores the pixels in Morton order, y bit lowest; a non-square image (0x0d) is a
 * row of twiddled squares. VQ data (3, and 4 with mipmaps) is a 256-entry codebook of 2x2 blocks
 * (4 pixels each, twiddled) and one byte per block, the blocks twiddled. Rectangles (9) are
 * plain rows. Mipmap chains go from 1x1 up; the full size comes last in twiddled chains (any
 * padding is in front), and VQ chains are padded at the end. */
#include <stdlib.h>
#include <string.h>

#include "pc/dc_textures.h"
#include "pc_dctex.h"
#include "pc_runtime.h"
#include "pc/gl_gpu.h"

namespace pc {

namespace {

uint32_t rd32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
void wr32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }

} // namespace

DcTextures g_dcTextures;

uint16_t *DcTextures::Slot(uint32_t checksum)
{
    uint32_t i = (checksum * 2654435761u) >> 18;   /* 14 bits */
    while (m_hash[i] && m_tex[m_hash[i]].checksum != checksum)
        i = (i + 1) & (kMaxHd * 2 - 1);
    return &m_hash[i];
}

void DcTextures::Add(uint32_t checksum, int w, int h, int pf, int df, const uint8_t *data,
                     uint32_t size)
{
    uint16_t *s = Slot(checksum);
    if (*s)
        return;   /* same checksum, same image */
    if (m_last + 1 >= kMaxHd - 1) {   /* the last handle is pc_gpu_gl.cpp's */
        PC_Log("dc: too many textures");
        return;
    }
    HdTex *t = &m_tex[m_last + 1];
    t->checksum = checksum;
    t->w = (uint16_t)w;
    t->h = (uint16_t)h;
    t->pf = (uint8_t)pf;
    t->df = (uint8_t)df;
    t->size = size;
    t->data = (uint8_t *)malloc(size);
    memcpy(t->data, data, size);
    t->refs = 0;
    *s = (uint16_t)++m_last;
}

void DcTextures::Ref(uint16_t handle, int delta)
{
    if (!handle || handle > m_last)
        return;
    m_tex[handle].refs += delta;
    if (m_tex[handle].refs <= 0) {
        m_tex[handle].refs = 0;
        g_glGpu.HdForget(handle);
    }
}

bool DcTextures::PsxIsV6(const uint8_t *data, uint32_t size)
{
    return size >= 12 && rd16(data) == 6;
}

/* ---- PowerVR images ------------------------------------------------------------------------ */

namespace {

uint32_t Twiddle(uint32_t x, uint32_t y)
{
    uint32_t i = 0;
    for (int b = 0; b < 11; b++)
        i |= ((y >> b) & 1) << (2 * b) | ((x >> b) & 1) << (2 * b + 1);
    return i;
}

void Pixel(uint8_t *o, uint16_t p, int pf)
{
    switch (pf) {
    case 0:
        o[0] = (uint8_t)((p >> 10 & 31) << 3 | (p >> 12 & 7));
        o[1] = (uint8_t)((p >> 5 & 31) << 3 | (p >> 7 & 7));
        o[2] = (uint8_t)((p & 31) << 3 | (p >> 2 & 7));
        o[3] = p & 0x8000 ? 255 : 0;
        break;
    case 2:
        o[0] = (uint8_t)((p >> 8 & 15) * 17);
        o[1] = (uint8_t)((p >> 4 & 15) * 17);
        o[2] = (uint8_t)((p & 15) * 17);
        o[3] = (uint8_t)((p >> 12 & 15) * 17);
        break;
    default:
        o[0] = (uint8_t)((p >> 11 & 31) << 3 | (p >> 13 & 7));
        o[1] = (uint8_t)((p >> 5 & 63) << 2 | (p >> 9 & 3));
        o[2] = (uint8_t)((p & 31) << 3 | (p >> 2 & 7));
        o[3] = 255;
        break;
    }
}

/* bytes of the mipmap levels below w x w: 16-bit texels, or VQ indices (one per 2x2 block, the
 * 1x1 level taking one) */
uint32_t MipBytes(int w, int vq)
{
    uint32_t n = 0;
    for (int m = 1; m < w; m *= 2)
        n += vq ? (m < 2 ? 1 : (uint32_t)(m / 2) * (m / 2)) : (uint32_t)m * m * 2;
    return n;
}

} // namespace

bool DcTextures::Decode(uint16_t handle, int *pw, int *ph, uint8_t **out)
{
    if (!handle || handle > m_last)
        return false;
    const HdTex *t = &m_tex[handle];
    int w = t->w, h = t->h, df = t->df;
    const uint8_t *d = t->data;
    uint32_t size = t->size;
    uint8_t *rgba = (uint8_t *)malloc((size_t)w * h * 4);
    bool ok = true;
    if (df == 1 || df == 2 || df == 0x0d || df == 0x12) {
        uint32_t need = (uint32_t)w * h * 2;
        uint32_t off = df == 1 || df == 0x0d ? 0 : size - need;
        int s = w < h ? w : h;
        if (size < need || off + need > size) {
            ok = false;
        } else {
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++) {
                    uint32_t tile = w > h ? (uint32_t)(x / s) : (uint32_t)(y / s);
                    uint32_t i = tile * s * s + Twiddle(x % s, y % s);
                    Pixel(rgba + ((size_t)y * w + x) * 4, rd16(d + off + i * 2), t->pf);
                }
        }
    } else if (df == 3 || df == 4) {
        uint32_t off = 2048 + (df == 4 ? MipBytes(w, 1) : 0);
        if (w != h || size < off + (uint32_t)(w / 2) * (h / 2)) {
            ok = false;
        } else {
            uint8_t book[256][4][4];
            for (int c = 0; c < 256; c++)
                for (int k = 0; k < 4; k++)
                    Pixel(book[c][k], rd16(d + (c * 4 + k) * 2), t->pf);
            for (int y = 0; y < h / 2; y++)
                for (int x = 0; x < w / 2; x++) {
                    const uint8_t (*b)[4] = book[d[off + Twiddle(x, y)]];
                    /* the block's texels: (0,0) (0,1) (1,0) (1,1) */
                    memcpy(rgba + ((size_t)(2 * y) * w + 2 * x) * 4, b[0], 4);
                    memcpy(rgba + ((size_t)(2 * y + 1) * w + 2 * x) * 4, b[1], 4);
                    memcpy(rgba + ((size_t)(2 * y) * w + 2 * x + 1) * 4, b[2], 4);
                    memcpy(rgba + ((size_t)(2 * y + 1) * w + 2 * x + 1) * 4, b[3], 4);
                }
        }
    } else if (df == 9) {
        if (size < (uint32_t)w * h * 2)
            ok = false;
        else
            for (int i = 0; i < w * h; i++)
                Pixel(rgba + (size_t)i * 4, rd16(d + i * 2), t->pf);
    } else {
        ok = false;
    }
    if (!ok) {
        if (m_decodeWarned++ < 8)
            PC_Log("dc: texture %08x: can't decode format %d/%#x (%dx%d, %u bytes)", t->checksum,
                   t->pf, df, w, h, size);
        free(rgba);
        return false;
    }
    *pw = w;
    *ph = h;
    *out = rgba;
    return true;
}

/* ---- version 6 files ----------------------------------------------------------------------- */

uint8_t *DcTextures::PsxStrip(const uint8_t *data, uint32_t size, uint32_t *outSize)
{
    /* the bytes [o, o + n) are in the file */
    auto in = [size](uint64_t o, uint64_t n) { return o + n <= size; };
    auto bad = [] {
        PC_Log("dc: version 6 file doesn't parse");
        return (uint8_t *)nullptr;
    };
    if (!in(0, 12))
        return bad();
    uint32_t nobj = rd32(data + 8), p = 12 + nobj * 36;
    if (!in(p, 4))
        return bad();
    uint32_t nmodels = rd32(data + p);
    /* the packets */
    uint32_t q = rd32(data + 4);
    for (;;) {
        if (!in(q, 4))
            return bad();
        if (rd32(data + q) == 0xffffffffu)
            break;
        if (!in(q, 8))
            return bad();
        q += 8 + rd32(data + q + 4);
    }
    q += 4 + nmodels * 4;
    if (!in(q, 4))
        return bad();
    uint32_t nchk = rd32(data + q);
    uint32_t chk = q + 4;
    q = chk + nchk * 4;
    if (!in(q, 4))
        return bad();
    q += 4 + rd32(data + q) * 36;
    if (!in(q, 4))
        return bad();
    q += 4 + rd32(data + q) * 516;
    if (!in(q, 4))
        return bad();
    uint32_t table = q, ntex = rd32(data + q);
    if (!in(table + 4, ntex * 4))
        return bad();

    /* the new image: everything before the texture table, the table, then the records without
     * their images (except any that aren't 16-bit, which the game uploads itself) */
    uint32_t cap = table + 4 + ntex * 4;
    for (uint32_t i = 0; i < ntex; i++) {
        uint32_t o = rd32(data + table + 4 + i * 4);
        if (!in(o, 28))
            return bad();
        if (rd32(data + o + 4) != 65536) {
            uint32_t w = rd16(data + o + 16), h = rd16(data + o + 18);
            uint32_t n = rd32(data + o + 4) == 256 ? ((w + 1) & ~1u) * h : ((w + 3) & ~3u) / 2 * h;
            if (!in(o + 20, n))
                return bad();
            cap += 20 + ((n + 3) & ~3u);
        } else {
            cap += 28;
        }
    }
    uint8_t *out = (uint8_t *)malloc(cap);
    memcpy(out, data, table);
    wr32(out + table, ntex);
    uint32_t w_at = table + 4 + ntex * 4;
    for (uint32_t i = 0; i < ntex; i++) {
        uint32_t o = rd32(data + table + 4 + i * 4);
        wr32(out + table + 4 + i * 4, w_at);
        if (rd32(data + o + 4) != 65536) {
            uint32_t w = rd16(data + o + 16), h = rd16(data + o + 18);
            uint32_t n = rd32(data + o + 4) == 256 ? ((w + 1) & ~1u) * h : ((w + 3) & ~3u) / 2 * h;
            memcpy(out + w_at, data + o, 20 + n);
            w_at += 20 + ((n + 3) & ~3u);
            continue;
        }
        uint32_t idx = rd32(data + o + 12), isize = rd32(data + o + 24);
        memcpy(out + w_at, data + o, 28);
        wr32(out + w_at + 24, 8);
        w_at += 28;
        if (idx >= nchk || isize < 8 || (uint64_t)o + 20 + isize > size) {
            PC_Log("dc: bad texture record %u", i);
            continue;
        }
        Add(rd32(data + chk + idx * 4), rd16(data + o + 16), rd16(data + o + 18), data[o + 20],
            data[o + 21], data + o + 28, isize - 8);
    }
    *outSize = w_at;
    return out;
}

/* ---- the Texture entries Dreamcast faces name ---------------------------------------------- */

uint32_t DcTextures::FaceTexIndex(void *texture)
{
    for (uint32_t i = 0; i < m_nfaceTex; i++)
        if (m_faceTex[i] == texture)
            return PC_DC_FACE_TEX | i;
    if (m_nfaceTex == kMaxFaceTex)
        PC_Fatal("more than %d Dreamcast texture entries", (int)kMaxFaceTex);
    m_faceTex[m_nfaceTex] = texture;
    return PC_DC_FACE_TEX | m_nfaceTex++;
}

void *DcTextures::FaceTex(uint32_t word) const
{
    uint32_t i = word & ~PC_DC_FACE_TEX;
    return i < m_nfaceTex ? m_faceTex[i] : nullptr;
}

} // namespace pc

/* ---- the game's interface (pc_dctex.h) ---- */

extern "C" {

uint16_t PC_HdTexFind(uint32_t checksum) { return pc::g_dcTextures.Find(checksum); }
void PC_HdTexRef(uint16_t handle, int delta) { pc::g_dcTextures.Ref(handle, delta); }
uint32_t PC_DcFaceTexIndex(void *texture) { return pc::g_dcTextures.FaceTexIndex(texture); }
void *PC_DcFaceTex(uint32_t word) { return pc::g_dcTextures.FaceTex(word); }

}
