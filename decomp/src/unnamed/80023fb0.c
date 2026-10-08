/* FUN_80023fb0 (0x80023fb0) */
int GameFMV_MusicScale;

int FUN_80023fb0(int a) {
	int t = (a * GameFMV_MusicScale) / 128;

	if (t > 16383)
		t = 16383;

	return t;
}
