/* Card_ReadFileHeader__FPcP9SCardHead (0x8001508c) */
#include "names.h"

extern char s_bu00_s[];

void FUN_8008b900(char *, char *, char *);
int FUN_8008eb4c(char *, int);
void _clear_event(void);
int FUN_8008ead8(int, void *, int);
int FUN_8008e934(int);
void Pause__Fi(int);
int Card_CheckStatus(int, int);

#ifdef PC_PORT
#include "pc_saves.h"
#endif

int Card_ReadFileHeader(char *name, short *out) {
	char buf[32];
	int i, fd, n;

#ifdef PC_PORT
	/* PC: the header of a listed save is just its name (pc_saves.h) */
	PC_SaveCardHeader(out, PC_SaveListType(), name);
	return 0;
#endif

	FUN_8008b900(buf, s_bu00_s, name);

	for (i = 0; i < 8; i++) {
		fd = FUN_8008eb4c(buf, 1);

		if (fd == -1) {
			*out = 26214;
			Pause__Fi(1);
			Card_CheckStatus(0, 0);
		} else {
			_clear_event();

			n = FUN_8008ead8(fd, out, 512);
			FUN_8008e934(fd);

			if (n == 512)
				return 0;
		}
	}

	return 1;
}
