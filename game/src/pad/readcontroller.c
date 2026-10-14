/* ReadController__FP14ControllerDataPUc (0x80070e5c) */
typedef struct {
	unsigned char b[8];
} ControllerData;

extern char Pad[];

void FUN_80096070(void);
void FUN_80096050(void);

void ReadController(ControllerData *d, unsigned char *s) {
	unsigned char old = d->b[1];

	d->b[0] = s[0];
	d->b[1] = s[1];
	d->b[2] = s[2];
	d->b[3] = s[3];
	d->b[4] = s[4];
	d->b[5] = s[5];
	d->b[6] = s[6];
	d->b[7] = s[7];

	if (s[0] == 0xff) {
		d->b[1] = 0;
	} else if (old != d->b[1]) {
		FUN_80096070();
		FUN_80096050();

		Pad[0x157] = 0;
		Pad[0x2c3] = 0;
	}
}
