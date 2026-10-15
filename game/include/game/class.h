/* CClass: the polymorphic base of the game's small heap objects (GLOBALS.H). No data: the vptr is
   at +0 (inserted by the port build / placed there by egcs). Its own operator new (OB.CPP) zeroes
   the block; operator delete frees through the memory manager.

   The destructor is inline (demo GLOBALS.H:92), and CClass has no out-of-line virtual, so every
   TU that needs CClass's vtable emits its own copy of it and of ~CClass: retail has five
   (0x800a1350, 0x800a21b0, 0x800a3464, 0x800a3484, 0x800a35c4, slot 0 = that TU's ~CClass). */
#ifndef GAME_CLASS_H
#define GAME_CLASS_H

/* The inline members and vtables of the game headers' classes are not emitted in every TU that
   uses them: the TUs refer to the vtables in game/data (egcs layout). The data files themselves
   build without it (GAME_DATA_TU), which gives their objects the classes' full debug types. */
#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface
#endif

#include "game/types.h"

class CClass {
  public:
	virtual ~CClass() {}                          /* slot 0 */
	static void *operator new(__SIZE_TYPE__ size) throw(); /* 0x800241dc */
	void operator delete(void *);                 /* 0x80024238 */
};

#endif
