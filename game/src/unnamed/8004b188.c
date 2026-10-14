/* FUN_8004b188 (0x8004b188) */
#include "names.h"
#include "pc_sound.h"

/* Loads a VAB sound bank: reads the file, sends the sample body (after the header and the
   512-byte-per-program tables) to SPU memory, then shrinks the buffer to the header. The SPU
   address goes into the header's size word. Called by FUN_8004b0e4 with "name.VAB". */
typedef struct {
	char pad0[12];
	int Size; /* 0x0c */
	char pad10[2];
	unsigned short NumProgs; /* 0x12 */
} SVabHead;

int FileIO_Open(char *);
void *Mem_NewX(int, int, int);
void FileIO_Load(void *);
void FileIO_Sync(void);
int FUN_800949a4(int);
void FUN_800938b4(int);
void FUN_80093df4(void *, int);
void FUN_80094d94(int);
void Mem_Shrink(void *, int);

int FUN_8004b188(char *name, SVabHead **out) {
	SVabHead *vab = Mem_NewX(FileIO_Open(name) + 64, 1, 1);
	int head, body, addr;

	FileIO_Load(vab);
	FileIO_Sync();

	head = (vab->NumProgs << 9) + 2592;
	body = (vab->Size - head + 63) & ~63;
	addr = PC_SoundBankLoad((char *)vab + head, body);

	vab->Size = addr;
	Mem_Shrink(vab, head);

	*out = vab;
	return 1;
}
