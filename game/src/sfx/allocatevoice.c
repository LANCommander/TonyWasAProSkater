/* allocateVoice__Fv (0x8004bef4) */
unsigned voicefree_flag;

int allocateVoice(void) {
	int i;

	for (i = 0; i < 24; i++) {
		if (voicefree_flag & (1 << i))
			return i;
	}

	return -1;
}
