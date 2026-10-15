/* Spool_FindAnim__FPc (0x80070064) */
/* C++ (SPOOL.CPP). Demo twin (identical opcodes); locals and blocks from its MAIN.SYM. Searches the loaded
   anim packets for an anim by name (case-insensitive, up to 8 characters). The masks sit in the
   assignments (line table) and cross-jumping merges the two pairs; testing `!lettera` in the break
   (CSE turns it into letterb's register) lets jump threading send that exit straight to found = 1. */
#include "game/m3d.h"
#include "game/spool.h"

AnimPacket *AnimPackets;

SAnimFrame *Spool_FindAnim(char *pName) {
	SAnimFrame *pAnim = 0;
	AnimPacket *pPacketInfo;

	for (pPacketInfo = AnimPackets; pPacketInfo; pPacketInfo = pPacketInfo->pNext) {
		uint NumAnims = *pPacketInfo->pPacket;
		char *pAnimName = (char *)(pPacketInfo->pPacket + 1);
		int found = 0;
		uint FirstFrame = 1; /* (the port's: the anim's first frame in pFrames, after its count) */

		{
			int i;

			for (i = 0; i < NumAnims; i++) {
				char lettera;
				char letterb;
				char *pA = pName;
				char *pB = pAnimName;
				int count = 0;
				uint NumFrames;

				lettera = *pA & 0xdf;
				letterb = *pB & 0xdf;

				while (1) {
					if (lettera != letterb || !lettera || count >= 8)
						break;

					pA++;
					pB++;
					lettera = *pA & 0xdf;
					letterb = *pB & 0xdf;
					count++;
				}

				if ((!lettera && !letterb) || count == 8) {
					found = 1;
					break;
				}

				NumFrames = *(uint *)(pAnimName + 8);
				pAnimName += NumFrames * 8 + 12;
				FirstFrame += NumFrames + 1;
			}
		}

		if (found) {
			pAnim = pPacketInfo->pFrames + FirstFrame;
			break;
		}
	}

	return pAnim;
}
