/* DumpAutoTest__Fv (0x800240d8) */
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
int FUN_80091bb0(char *);
int FUN_80091894(int);

void DumpAutoTest(void) {
	char pad[24];
	char env[96];
	char buf[200];
	int id;

	FUN_8008c3d0(1);
	FUN_8008c314(0);
	FUN_8009153c(0, 0);

	id = FUN_800915dc(200, 175, 160, 16, 1, 1024);
	DrawSync(0);
	FUN_80092e70(env, 0, 0, 512, 240);
	FUN_8008c94c(env);
	sprintf(buf, "L-%d, P1-%d, P2-%d, G-%d", GAutoLevela, GAutoPlayer1a, GAutoPlayer2a, GAutoGamea);
	FUN_80091bb0(buf);
	FUN_80091894(id);

	GAutoLevela = GAutoLevel;
	GAutoPlayer1a = GAutoPlayer1;
	GAutoPlayer2a = GAutoPlayer2;
	GAutoGamea = GAutoGame;
}
