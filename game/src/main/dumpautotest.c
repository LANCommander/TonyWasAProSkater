/* DumpAutoTest__Fv (0x800240d8) */
#include "pc_screen.h"
int GAutoLevela, GAutoPlayer1a, GAutoPlayer2a, GAutoGamea;
int GAutoLevel, GAutoPlayer1, GAutoPlayer2, GAutoGame;

int FUN_8008c3d0(int);
int FUN_8008c314(int);
int FUN_8009153c(int, int);
int FUN_800915dc(int, int, int, int, int, int);
int DrawSync(int);
void *FUN_80092e70(void *, int, int, int, int);
int FUN_8008c94c(void *);
int sprintf(char *, char *, ...);
int printf(const char *, ...);
int FUN_80091bb0(char *);
int FUN_80091894(int);

void DumpAutoTest(void) {
	char pad[24];
	char env[96];
	char buf[200];
	int id;

	/* PC: libgpu's debug font isn't drawn; the line goes to the trace log */
	PC_ScreenEnable(1);
	sprintf(buf, "L-%d, P1-%d, P2-%d, G-%d", GAutoLevela, GAutoPlayer1a, GAutoPlayer2a, GAutoGamea);
	printf("%s\n", buf);

	GAutoLevela = GAutoLevel;
	GAutoPlayer1a = GAutoPlayer1;
	GAutoPlayer2a = GAutoPlayer2;
	GAutoGamea = GAutoGame;
}
