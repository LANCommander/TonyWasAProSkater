/* Force-included ahead of every game TU (build.py: -include): the geometry maths (pc_geom.h), which
 * the game uses where the PS1 used the GTE, and the C++ glue below. */
#ifndef PC_GAME_H
#define PC_GAME_H

#include "pc_geom.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Delete statements in C++ TUs become pc_delete(p) (portsrc.mark_deletes). For a class whose
 * vtable has a destructor, the TU gets an exact-type overload that does what egcs does: call the
 * vtable destructor with in_chrg 3, which destroys and frees through the class allocator. Other
 * types use the template below, which records the pointer so the build's destructor glue can do
 * the same (see link.py). */
void pc_delete_mark(void *p);
void pc_vdelete(void *vtable, int slot, void *obj);

/* What portsrc.py makes of a GTE instruction (inline asm) in the game's code: an error. The port
 * has no GTE; use the geometry maths (pc_geom.h). */
void PC_GteAsmInPcBuild(void)
    __attribute__((error("GTE asm in the game's code: use the geometry maths (pc_geom.h)")));
#ifdef __cplusplus
}
template <class T> static inline void pc_delete(T *p) {
    pc_delete_mark((void *)p);
    delete p;
}
#endif

#endif
