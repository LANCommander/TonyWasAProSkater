/* FUN_80077130 (0x80077130) */
/* C++ (COMPETITION). Out-of-line destructor of the class in FUN_80077060 (retail vtable 0x800a3444
   points here). The real class derives from CClass and its destructor is the implicit one, which
   never re-stores its own vptr. A standalone file can't make cc1 emit that synthesized copy
   without also emitting the static ctor/dtor first, so this uses a stand-in with a CClass member
   at +0. It compiles to the same code: the array of five polymorphic elements at +40 is destroyed
   through their vtables, then the inlined ~CClass vptr store. */
#include "game/class.h"
#include "game/competition.h"

class X {
  public:
	CClass c;
	char pad[36];
	CJudge e[5];
	~X();
};

X::~X() {}
