/* Decompiled data: the retail executable's initialised globals, in C.
 *
 * Each global is defined with its PS1 address: PSX_DATA puts it in a section of its own named
 * after that address, and platform/pc/tools/data_image.py places every such section at its
 * address in the port's data image (resolving the pointers between them to PS1 addresses too).
 * The files are compiled for the port only (native gcc -m32: little-endian, 4-byte int, long and
 * pointers, as on the PS1), never by the matching toolchain; the decompiled code declares the
 * globals it uses itself.
 *
 * `extern char Name[];` declares anything a pointer refers to: a function, a global in another
 * file or in .bss. data_image.py finds its address by name (functions.tsv, these files, the TUs'
 * .syms.json, data_names.tsv) or from a `_<8 hex digits>` suffix (D_800d3000, FUN_8008f1a0). */
#ifndef PSXDATA_H
#define PSXDATA_H

#define PSX_DATA(addr) __attribute__((section(".psx" #addr), used))

/* A symbol whose name isn't a C identifier (egcs destructors `_._9CSmokeJet`, `_GLOBAL_.I.x`):
 * `extern char dtor_9CSmokeJet[] PSX_NAME("_._9CSmokeJet");` */
#ifdef _WIN32
#define PSX_NAME(name) __asm__("_" name) /* PE prefixes C names with '_' */
#else
#define PSX_NAME(name) __asm__(name)
#endif

#endif
