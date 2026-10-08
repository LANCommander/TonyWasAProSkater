/* Force-included ahead of every decompiled TU in the PC build (build.py: -include). The sources
 * themselves stay byte-for-byte the matching ones; portsrc.py rewrites their inline GTE asm into
 * calls declared here. */
#ifndef PC_GAME_H
#define PC_GAME_H

#include "pc_gte.h"

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
#ifdef __cplusplus
}
template <class T> static inline void pc_delete(T *p) {
    pc_delete_mark((void *)p);
    delete p;
}
#endif

#endif
