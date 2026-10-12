/* GLSL sources for the OpenGL GPU backend (pc_gpu_gl.c). Positions are VRAM coordinates, mapped
 * onto the render target's VRAM area (u_target: all of VRAM, or a display surface); the target's
 * rows run the way VRAM's do (no flip), so readbacks and scissors need no conversion.
 *
 * The PS1 decides coverage and evaluates attributes at a pixel's top-left corner, GL at its
 * centre. At 1x the geometry moves by half a pixel (u_shift), which makes GL sample exactly the
 * PS1's points. Scaled up, that shift would leave seams beside fills and CPU-drawn images, so
 * the grid stays and only texture coordinates are moved back to the corner (u_uvfix).
 *
 * Native 3D (info bit 0x4000, pc_render3d.c) has view-space positions: u_proj = (screen x and y
 * of the view axis incl. the drawing offset, focal length H, near plane). Depth is linear view
 * z / 65536. The PS1 draws in painter order, and coplanar surfaces (decals, shadows) rely on it,
 * so colour is tested a little in front of the true depth (u_dpass 0) and the true depth is
 * written in a second, depth-only pass (u_dpass 1): a later surface wins a tie, and anything
 * further than the tolerance stays hidden. Emulated primitives with a view depth (0x2000) are
 * tested the same way; the rest have depth 0 and always pass. 0x8000 (sky) is at depth 1.
 *
 * Colour depth 3 (info bits 7-8; the PS1 game never uses it) is an HD texture (pc_dctex.h), bound
 * to u_hd: 256 texture units = one repeat, filtered, its alpha (1555/4444 images) blending on top
 * of the PS1's semi-transparency. */
#ifndef PC_GPU_GL_SHADERS_H
#define PC_GPU_GL_SHADERS_H

static const char *gl_draw_vs =
    "#version 330 core\n"
    "layout(location = 0) in vec3 a_pos;\n"       /* x, y, w (perspective divisor) or view x, y, z */
    "layout(location = 1) in vec4 a_color;\n"
    "layout(location = 2) in vec2 a_uv;\n"
    "layout(location = 3) in uvec2 a_tex;\n"      /* clut word, info bits (see pc_gpu_gl.c) */
    "layout(location = 4) in uvec4 a_twin;\n"     /* texture window and_x, and_y, or_x, or_y */
    "layout(location = 5) in uvec4 a_uvr;\n"       /* texels the face spans: u, v min, max */
    "out vec3 v_color;\n"
    "out vec2 v_uv;\n"
    "flat out uvec2 v_tex;\n"
    "flat out uvec4 v_twin;\n"
    "flat out uvec4 v_uvr;\n"
    "out float v_z;\n"
    "uniform float u_shift;\n"
    "uniform vec4 u_proj;\n"
    "uniform vec4 u_target;\n"                    /* the VRAM area drawn on: x, y, w, h */
    "void main() {\n"
    "    vec2 k = 2.0 / u_target.zw;\n"
    "    if ((a_tex.y & 0x4000u) != 0u) {\n"
    "        vec3 v = a_pos;\n"
    "        vec2 o = (u_proj.xy + u_shift - u_target.xy) * k - 1.0;\n"
    "        gl_Position = vec4(o * v.z + u_proj.z * k * v.xy, 0.0, v.z);\n"
    "        gl_ClipDistance[0] = v.z - u_proj.w;\n"
    "        v_z = v.z;\n"
    "    } else {\n"
    "        vec2 p = (a_pos.xy + u_shift - u_target.xy) * k - 1.0;\n"
    "        gl_Position = vec4(p, 0.0, 1.0) * a_pos.z;\n"
    "        gl_ClipDistance[0] = 1.0;\n"
    "        v_z = (a_tex.y & 0x2000u) != 0u ? a_pos.z : 0.0;\n"
    "    }\n"
    "    v_color = a_color.rgb;\n"
    "    v_uv = a_uv;\n"
    "    v_tex = a_tex;\n"
    "    v_twin = a_twin;\n"
    "    v_uvr = a_uvr;\n"
    "}\n";

/* Output 0 is the colour (alpha = the VRAM mask bit), output 1's alpha the weight the old pixel
 * keeps (dual-source blending: dst * (1 - src1.a) + src). B-F is drawn in a separate pass with a
 * reverse-subtract equation: u_pass 1 draws only the opaque pixels, 2 only the blended ones. */
static const char *gl_draw_fs =
    "#version 330 core\n"
    "in vec3 v_color;\n"
    "in vec2 v_uv;\n"
    "flat in uvec2 v_tex;\n"
    "flat in uvec4 v_twin;\n"
    "flat in uvec4 v_uvr;\n"
    "in float v_z;\n"
    "uniform usampler2D u_vram;\n"
    "uniform sampler2D u_hd;\n"
    "uniform vec2 u_uvfix;\n"
    "uniform int u_pass;\n"
    "uniform int u_dpass;\n"
    "layout(location = 0, index = 0) out vec4 o_color;\n"
    "layout(location = 0, index = 1) out vec4 o_blend;\n"
    "uint vram(int x, int y) { return texelFetch(u_vram, ivec2(x & 1023, y & 511), 0).r; }\n"
    "void main() {\n"
    /* texture coordinates at the pixel's corner, rounded to the nearest texel like the PS1. On an
     * emulated primitive drawn smaller than its texture (a Dreamcast image laid out at the PS1's
     * size) the move would reach past its first texel: at most half a texel, which is what it
     * is at 1:1 */
    "    uint info = v_tex.y;\n"
    "    vec2 fix = u_uvfix.x * dFdx(v_uv) + u_uvfix.y * dFdy(v_uv);\n"
    "    if ((info & 0x4000u) == 0u) fix = clamp(fix, -0.5, 0.5);\n"
    "    vec2 uv = v_uv - fix + 0.5;\n"
    /* native faces stay inside their own texels (emulated rects and sprites rely on wrapping) */
    "    if ((info & 0x4000u) != 0u) uv = clamp(uv, vec2(v_uvr.xy), vec2(v_uvr.zw) + 0.999);\n"
    "    bool semi = (info & 0x800u) != 0u;\n"
    "    int abr = int((info >> 5) & 3u);\n"
    "    float mask = (info & 0x1000u) != 0u ? 1.0 : 0.0;\n"
    "    vec3 c = v_color;\n"
    "    bool blend = semi;\n"
    "    float hd_a = 1.0;\n"
    /* derivatives outside the branch: a 2x2 pixel quad can span primitives of both kinds */
    "    vec2 hd_uv = v_uv / 256.0, hd_dx = dFdx(hd_uv), hd_dy = dFdy(hd_uv);\n"
    "    if ((info & 0x380u) == 0x380u) {\n"
    "        vec4 t = textureGrad(u_hd, hd_uv, hd_dx, hd_dy);\n"
    "        if (t.a < 0.03) discard;\n"
    "        c = (info & 0x400u) != 0u ? t.rgb : min(t.rgb * v_color * (255.0 / 128.0), vec3(1.0));\n"
    "        hd_a = t.a;\n"
    "    } else if ((info & 0x200u) != 0u) {\n"
    "        int u = (int(floor(uv.x)) & 255 & int(v_twin.x)) | int(v_twin.z);\n"
    "        int v = (int(floor(uv.y)) & 255 & int(v_twin.y)) | int(v_twin.w);\n"
    "        int tpx = int(info & 15u) * 64, tpy = int((info >> 4) & 1u) * 256;\n"
    "        int depth = int((info >> 7) & 3u);\n"
    "        int cx = int(v_tex.x & 63u) * 16, cy = int((v_tex.x >> 6) & 511u);\n"
    "        uint t;\n"
    "        if (depth == 0) {\n"
    "            uint w = vram(tpx + (u >> 2), tpy + v);\n"
    "            t = vram(cx + int((w >> (uint(u & 3) * 4u)) & 15u), cy);\n"
    "        } else if (depth == 1) {\n"
    "            uint w = vram(tpx + (u >> 1), tpy + v);\n"
    "            t = vram(cx + int((w >> (uint(u & 1) * 8u)) & 255u), cy);\n"
    "        } else {\n"
    "            t = vram(tpx + u, tpy + v);\n"
    "        }\n"
    "        if (t == 0u) discard;\n"
    "        vec3 tc = vec3(float(t & 31u), float((t >> 5) & 31u), float((t >> 10) & 31u)) / 31.0;\n"
    "        c = (info & 0x400u) != 0u ? tc : min(tc * v_color * (255.0 / 128.0), vec3(1.0));\n"
    "        bool stp = (t & 0x8000u) != 0u;\n"
    "        blend = semi && stp;\n"
    "        if (stp) mask = 1.0;\n"
    "    }\n"
    "    if ((u_pass == 1 && blend) || (u_pass == 2 && !blend)) discard;\n"
    "    float keep = 0.0;\n"
    "    if (blend) {\n"
    "        if (abr == 0) { c *= 0.5; keep = 0.5; }\n"
    "        else if (abr == 3) { c *= 0.25; keep = 1.0; }\n"
    "        else keep = 1.0;\n"
    "    }\n"
    "    c *= hd_a;\n"
    "    keep = hd_a * keep + 1.0 - hd_a;\n"
    "    o_color = vec4(c, mask);\n"
    "    o_blend = vec4(0.0, 0.0, 0.0, 1.0 - keep);\n"
    "    if ((info & 0x8000u) != 0u) gl_FragDepth = 1.0;\n"
    "    else if ((info & 0x4000u) != 0u && u_dpass == 1) gl_FragDepth = clamp(v_z / 65536.0, 0.0, 1.0);\n"
    "    else if ((info & 0x6000u) != 0u)\n"
    "        gl_FragDepth = clamp((v_z * (1.0 - 1.0 / 256.0) - 2.0) / 65536.0, 0.0, 1.0);\n"
    "    else gl_FragDepth = 0.0;\n"
    "}\n";

/* CPU VRAM (the 16-bit atlas) into the render target, for cells the CPU wrote. */
static const char *gl_copy_fs =
    "#version 330 core\n"
    "uniform usampler2D u_vram;\n"
    "uniform vec4 u_map;\n"                         /* the target's VRAM x, y; pixels per texel */
    "out vec4 o_color;\n"
    "void main() {\n"
    "    ivec2 p = ivec2(floor(gl_FragCoord.xy / u_map.zw)) + ivec2(u_map.xy);\n"
    "    uint t = texelFetch(u_vram, p, 0).r;\n"
    "    o_color = vec4(vec3(float(t & 31u), float((t >> 5) & 31u), float((t >> 10) & 31u)) / 31.0,\n"
    "                   float(t >> 15));\n"
    "}\n";

static const char *gl_copy_vs =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_pos;\n"
    "uniform vec4 u_target;\n"
    "void main() {\n"
    "    gl_Position = vec4((a_pos - u_target.xy) * 2.0 / u_target.zw - 1.0, 0.0, 1.0);\n"
    "}\n";

/* The display area to the window. */
static const char *gl_present_vs =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_pos;\n"
    "layout(location = 1) in vec2 a_uv;\n"
    "out vec2 v_uv;\n"
    "void main() {\n"
    "    gl_Position = vec4(a_pos, 0.0, 1.0);\n"
    "    v_uv = a_uv;\n"
    "}\n";

static const char *gl_present_fs =
    "#version 330 core\n"
    "in vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "out vec4 o_color;\n"
    "void main() {\n"
    "    o_color = vec4(texture(u_tex, v_uv).rgb, 1.0);\n"
    "}\n";

#endif
