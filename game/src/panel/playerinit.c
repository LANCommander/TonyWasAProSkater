/* Panel_PlayerInit__FUsP6CBruce (0x8006e144) */
extern char *MechList;

void Panel_PlayerInit(unsigned short n, char *b) {
	if (b == MechList)
		*(short *)(b + 0x92c) = 0;
	else
		*(short *)(b + 0x92c) = 1;
}
