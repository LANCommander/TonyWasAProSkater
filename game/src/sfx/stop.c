unsigned nko_flag, loop_flag, voiceon_flag, keyon_queue, keyoff_queue;

void SFX_Stop(unsigned m) {
	unsigned inv = ~m;

	nko_flag &= inv;
	loop_flag &= inv;
	voiceon_flag &= inv;
	keyon_queue &= inv;
	keyoff_queue |= m;
}
