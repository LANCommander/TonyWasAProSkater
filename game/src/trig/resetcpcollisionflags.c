/* Trig_ResetCPCollisionFlags__Fi (0x80044994) */
typedef struct CP {
	int pad0;
	unsigned char coll, exec;
	char pad6[14];
	struct CP *next;
} CP;

CP *CommandPoints;

void Trig_ResetCPCollisionFlags(int m) {
	CP *p;

	for (p = CommandPoints; p; p = p->next)
		p->coll &= ~m;
}
