/* M3dInit_SetResolution__FUiUi (0x800820e4) */
extern unsigned Xres __attribute__((section(".data"))), Yres __attribute__((section(".data")));
extern unsigned PixelAspectX __attribute__((section(".data"))),
    PixelAspectY __attribute__((section(".data")));

void M3dInit_SetResolution(unsigned w, unsigned h) {
	Xres = w;
	Yres = h;

	if (w * 3 > h * 4) {
		PixelAspectX = 4096;
		PixelAspectY = (w * 3 << 12) / (h * 4);
	} else {
		PixelAspectY = 4096;
		PixelAspectX = (h * 4 << 12) / (w * 3);
	}
}
