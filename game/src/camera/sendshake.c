/* SendShake__FRC7CVectorP5CBody10EShakeType (0x800789a8) */
void SendShake(void *v, char *b, int type) {
	while (b) {
		*(unsigned short *)(b + 0x50) |= 2;
		b = *(char **)(b + 0x1c);
	}
}
