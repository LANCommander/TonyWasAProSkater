/* Decompiled data: the retail executable's initialised globals, in C.
 *
 * Each global carries its PS1 address. PSX_DATA globals are ordinary C globals in the port: the
 * address only names them (a section of their own named after it), so that the decompiled code,
 * which reaches globals by retail address (`psx_<addr>`, its .syms.json), links to them, and so
 * that platform/pc/tools/data_image.py can check them against the retail bytes. Text is string
 * literals, written where it's used. The files are compiled for the port only (native gcc -m32:
 * little-endian, 4-byte int, long and pointers, as on the PS1), never by the matching toolchain;
 * the decompiled code declares the globals it uses itself.
 *
 * PSX_FIXED globals (decomp/data/fixed/) stay at their retail address in PS1 RAM: data_image.py
 * builds them into the image the port copies to 0x80010000. That's for data whose address
 * matters: GPU packets and ordering tables (24-bit links), arrays the code indexes past the end
 * of into their neighbours, vtables (the port stores vptrs as numbers). They can't point at
 * native data or literals.
 *
 * `extern char Name[];` declares anything a pointer refers to outside the file: a function, a
 * global in another file or in .bss. data_image.py finds its address by name (functions.tsv,
 * these files, the TUs' .syms.json, data_names.tsv) or from a `_<8 hex digits>` suffix
 * (D_800d3000, FUN_8008f1a0). */
#ifndef PSXDATA_H
#define PSXDATA_H

#ifdef _WIN32
#define PSX_DATA(addr) __attribute__((section(".data$psx" #addr), used))
#else
#define PSX_DATA(addr) __attribute__((section(".data.psx" #addr), used))
#endif
#define PSX_FIXED(addr) __attribute__((section(".psx" #addr), used))

/* A symbol whose name isn't a C identifier (egcs destructors `_._9CSmokeJet`, `_GLOBAL_.I.x`):
 * `extern char dtor_9CSmokeJet[] PSX_NAME("_._9CSmokeJet");` */
/* An egcs vtable entry: the `this` adjustment, the index and the function (the first entry of a
 * vtable is all zeros). 8 bytes in the retail layout. */
struct PsxVtEntry {
	short delta, index;
	void *pfn;
};

/* Zero storage for an object of class T that its code constructs (a static constructor), sized as
 * the port's T (a CClass, whose vptr is first in both layouts) without being one. */
template <class T> union PsxStorage {
	char bytes[sizeof(T)];
	void *align_;
};

/* A CQuat (game/vector.h, a CClass) as zero storage: its vptr, then x, y, z, w. */
struct PsxQuat {
	void *vptr;
	int x, y, z, w;
};

#define PSX_STR_(x) #x
#define PSX_STR(x) PSX_STR_(x)
/* with the target's prefix for C names: '_' on 32-bit Windows, none on x64 or Linux */
#define PSX_NAME(name) __asm__(PSX_STR(__USER_LABEL_PREFIX__) name)

#endif
