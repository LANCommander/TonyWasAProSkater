/* SkipCommand__FPUs (0x8004601c) */
/* C++ (TRIG.CPP). The case groups were fitted to retail's compare tree with tools/casetree.py; the
   empty groups (10-12, 0x88-0x89, 0x95 sharing the default label) only show up in its shape. */
#include "game/types.h"

char *SkipString(char *p);

ushort *SkipCommand(ushort *pCommand) {
	switch (*pCommand++) {
		case 0xffff:
			return 0;

		case 0x82:
		case 0x87:
		case 0x8b:
		case 0x8f:
		case 0x90:
		case 0x91:
		case 0x92:
		case 0xa7:
		case 0xae:
		case 0xc8:
		case 0xca:
			pCommand += 2;
			break;

		case 0x68:
			pCommand += 3;
			break;

		case 0x73:
		case 0x77:
		case 0x7e:
		case 0x7f:
		case 0x80:
		case 0x8c:
		case 0x8e:
		case 0xb0:
		case 0xb2:
			pCommand = (ushort *)SkipString((char *)pCommand);
			break;

		case 2:
			while (*(char *)pCommand)
				pCommand = (ushort *)SkipString((char *)pCommand);

			pCommand++;
			break;

		case 0xab: {
			uint *p_checksum = (uint *)(((int)pCommand + 3) & ~3);

			pCommand = (ushort *)p_checksum + 5;
			break;
		}

		case 0xc9: {
			uint *p_checksum = (uint *)(((int)pCommand + 3) & ~3);

			pCommand = (ushort *)p_checksum + 3;
			break;
		}

		case 0x8d:
			pCommand += 2;
		case 0x85: {
			while (*pCommand != 0xff) {
				int *p_pos = (int *)(((int)pCommand + 3) & ~3);

				p_pos += 6;
				pCommand = (ushort *)p_pos;
			}

			pCommand++;
			break;
		}

		case 0x0d:
		case 0x69:
		case 0x6a:
		case 0x83:
		case 0x84:
		case 0x86:
		case 0x8a:
		case 0x93:
		case 0x94:
		case 0x99:
		case 0x9a:
		case 0x9b:
		case 0x9c:
		case 0xa0:
		case 0xa3:
		case 0xa4:
		case 0xa5:
		case 0xa6:
		case 0xa8:
		case 0xa9:
		case 0xaa:
		case 0xac:
		case 0xb1:
			pCommand++;
			break;

		case 3:
		case 4:
		case 5:
		case 0x66:
		case 0x67:
		case 0x79:
		case 0x7a:
		case 0x81:
		case 0xad:
		case 0xaf:
			break;

		case 0xa:
		case 0xb:
		case 0xc:
			return pCommand;
		case 0x88:
		case 0x89:
			return pCommand;
		case 0x95:
		default:
			return pCommand;
	}

	return pCommand;
}
