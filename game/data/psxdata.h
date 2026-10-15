/* Decompiled data: the retail executable's initialised globals, in C.
 *
 * Ordinary C++ globals, compiled and linked with the game (build.py), which the decompiled code
 * reaches by name. Text is string literals, written where it's used. The game code declares the
 * globals it uses itself; bss.cpp holds the zero-initialised ones no module's file has.
 *
 * `extern char Name[];` declares anything a pointer refers to outside the file: a function or a
 * global in another file. Functions are named as game/functions.tsv names them (or FUN_<addr>);
 * build.py links those references to the game's code. */
#ifndef PSXDATA_H
#define PSXDATA_H

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
