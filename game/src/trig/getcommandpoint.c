/* GetCommandPoint__Fi (0x80044920) */
typedef struct CP {
	int pad0;
	unsigned char coll, exec;
	short pad6;
	short pad8;
	unsigned short id;
	char padc[8];
	struct CP *next;
} CP;

short **OffsetList;
CP *CommandPoints;

CP *GetCommandPoint(int n) {
	CP *p;

	if (n != 0xffff && *OffsetList[n] == 6) {
		for (p = CommandPoints; p; p = p->next)
			if (p->id == n)
				return p;
	}

	return 0;
}
