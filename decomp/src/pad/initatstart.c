/* Pad_InitAtStart__Fv (0x80070878) */
#include "names.h"

typedef struct {
	char c[0x10];
} SButton;

typedef struct {
	SButton a, b, c, d;
	char _40[0x15c - 0x40];
	SButton *pa, *pc, *pb, *pd;
} SControl;

extern SControl Pad[];
extern char controllers[];

int Pad_IdleTime, ControlSystemInitialised;

extern void Pad_ClearAll(void);
extern void FUN_80096304(char *, char *);
extern void FUN_80096050(void);
extern void ResetMainDemoTimer(void);

void Pad_InitAtStart(void) {
	int i;

	for (i = 0; i < 2; i++) {
		Pad[i].pa = &Pad[i].a;
		Pad[i].pb = &Pad[i].c;
		Pad[i].pc = &Pad[i].b;
		Pad[i].pd = &Pad[i].d;
	}

	Pad_ClearAll();
	FUN_80096304(controllers, controllers + 34);
	FUN_80096050();

	Pad_IdleTime = 0;
	ResetMainDemoTimer();

	ControlSystemInitialised = 1;
}
