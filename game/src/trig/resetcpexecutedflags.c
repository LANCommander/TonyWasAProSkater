/* Trig_ResetCPExecutedFlags__Fi (0x800449cc) */
typedef struct CP {
	int pad0;
	unsigned char coll, exec;
	char pad6[14];
	struct CP *next;
} CP;

CP *CommandPoints;

void Trig_ResetCPExecutedFlags(int m) {
	CP *p;

	for (p = CommandPoints; p; p = p->next)
		if ((p->exec & m) && !(p->coll & m))
			p->exec &= ~m;
}
