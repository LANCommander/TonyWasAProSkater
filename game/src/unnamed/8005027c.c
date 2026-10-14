/* FUN_8005027c (0x8005027c) */
void FUN_800502bc(int a);
void FUN_800507e4(int a, int b);

void FUN_8005027c(int a, int b, int c) {
	if (a == 0)
		FUN_800502bc(b);
	else if (a == 1)
		FUN_800507e4(b, c);
}
