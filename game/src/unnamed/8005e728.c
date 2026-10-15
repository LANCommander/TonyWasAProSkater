/* FUN_8005e728 (0x8005e728) */
extern int DAT_800c0d00[];

struct VerticalMenu;

void FUN_800648c0(struct VerticalMenu *a, int b, int c);

void FUN_8005e728(struct VerticalMenu *a) {
	FUN_800648c0(a, 1, DAT_800c0d00[0] == 0);
}
