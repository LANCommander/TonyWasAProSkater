extern void FUN_80096268(int, int, int);

void f() {
#ifndef PC_PORT /* PC: the pads have no analog mode to lock (pc_input.h) */
	FUN_80096268(0, 1, 3);
	FUN_80096268(0x10, 1, 3);
#endif
}
