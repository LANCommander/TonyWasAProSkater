/* frug__FUi (0x800782b4) */
unsigned int CheatCheck;

int arccos(int);

void frug(unsigned int n) {
	CheatCheck ^= n;
	CheatCheck = (CheatCheck << 1) ^ (CheatCheck >> 31);
	CheatCheck = CheatCheck * 521;
	CheatCheck += arccos(CheatCheck);
}
