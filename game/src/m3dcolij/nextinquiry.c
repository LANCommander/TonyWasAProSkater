/* NextInquiry__Fv (0x8007ea94) */
typedef struct Item {
	short _0;
	unsigned short inquiry;
	char _4[0x18];
	struct Item *next;
} Item;

unsigned short Inquiry;

extern Item *EnviroList __attribute__((section(".data"))),
    *EnvironmentalObjectList __attribute__((section(".data")));

void NextInquiry(void) {
	Item *p;

	if (++Inquiry == 0) {
		Inquiry = 1;

		for (p = EnviroList; p; p = p->next)
			p->inquiry = 0;

		for (p = EnvironmentalObjectList; p; p = p->next)
			p->inquiry = 0;
	}
}
