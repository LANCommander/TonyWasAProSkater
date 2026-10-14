/* SendSignalToNode__FP5CBodyi (0x80045d30) */
void SendSignalToNode(char *b, int node) {
	while (b) {
		if (*(unsigned short *)(b + 0xe2) == node)
			*(unsigned short *)(b + 0x50) |= 1;

		b = *(char **)(b + 0x1c);
	}
}
