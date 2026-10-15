/* Dreamcast models and textures (src/pc_dctex.cpp; the format and the C interface are described
 * in pc_dctex.h): version 6 .psx files are loaded without their PowerVR texture images, which are
 * kept here by checksum and drawn by OpenGL, and the Texture entries Dreamcast faces name. */
#pragma once

#include <stdint.h>

namespace pc {

class DcTextures {
public:
    /* true if the file image is a version 6 .psx */
    static bool PsxIsV6(const uint8_t *data, uint32_t size);
    /* The file image without its texture images, which are registered here (malloc'd; *outSize).
     * Null if the file doesn't parse. */
    uint8_t *PsxStrip(const uint8_t *data, uint32_t size, uint32_t *outSize);

    /* Handle of a registered texture (0 = none). */
    uint16_t Find(uint32_t checksum) { return *Slot(checksum); }
    /* A texture entry starts or stops using it; at 0 users its OpenGL texture is freed. */
    void Ref(uint16_t handle, int delta);
    /* The texture's full-size image as RGBA8 rows (malloc'd), w x h; false if there is none. */
    bool Decode(uint16_t handle, int *w, int *h, uint8_t **rgba);

    /* A Dreamcast face's word 4: PC_DC_FACE_TEX | the index of its Texture entry. */
    uint32_t FaceTexIndex(void *texture);
    /* The Texture entry of a face word made by FaceTexIndex. */
    void *FaceTex(uint32_t word) const;

private:
    static constexpr int kMaxHd = 8192;
    static constexpr uint32_t kMaxFaceTex = 4096;

    struct HdTex {
        uint32_t checksum;
        uint16_t w, h;
        uint8_t pf, df;
        uint32_t size;      /* of the image data */
        uint8_t *data;
        int refs;
    };

    uint16_t *Slot(uint32_t checksum);
    void Add(uint32_t checksum, int w, int h, int pf, int df, const uint8_t *data, uint32_t size);

    HdTex m_tex[kMaxHd] = {};   /* handle = index; 0 unused */
    int m_last = 0;             /* the last handle given out (all zero: the object stays in bss) */
    uint16_t m_hash[kMaxHd * 2] = {};
    int m_decodeWarned = 0;
    /* The game's Texture entries that Dreamcast faces name (PC_DC_FACE_TEX | index). An entry the
     * game frees and another takes the place of gets the same index again. */
    void *m_faceTex[kMaxFaceTex] = {};
    uint32_t m_nfaceTex = 0;
};

extern DcTextures g_dcTextures;

} // namespace pc
