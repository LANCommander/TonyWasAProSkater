extern void Pad_ClearAllOne(int);

void f() {
	int i = 0;

	do {
		Pad_ClearAllOne(i);

		i++;
	} while (i < 2);
}
