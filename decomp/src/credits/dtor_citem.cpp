/* _._5CItem (0x8005bb3c) */
/* C++ (CREDITS.CPP). The credits module's own CItem (a CClass with its vptr at offset 0) has an
   empty destructor; deleting frees through CClass's operator delete. */
#include "game/class.h"

/* TU-local: really this TU's copy of CClass's inline destructor (build/hdr_notes/credits.md);
   kept under its retail symbol name */
class CItem : public CClass {
  public:
	virtual ~CItem();
};

CItem::~CItem() {}
