/* SetVariable__9CPlatformUs (0x8003f45c) */
#include "game/platform.h"

uint Spool_GetModel(uint, int);

void CPlatform::SetVariable(ushort variable) {
	switch (variable) {
		case 8483:
			mApplyGravity = *(uchar *)mpScript;
			mpScript++;
			break;
		case 8484:
			mModel = *mpScript++;
			break;
		case 8495: {
			uint *p_checksum = (uint *)(((intptr_t)mpScript + 3) & -4);

			mModel = Spool_GetModel(*p_checksum, mRegion);
			mpScript = (ushort *)(p_checksum + 1);
			mFlags &= ~1;
			break;
		}
		case 8500: {
			short a = GetScriptValue();
			short b = GetScriptValue();
			short c = GetScriptValue();

			mVel.Set(a << 12, b << 12, c << 12);
			break;
		}
		case 8503: {
			short a = GetScriptValue();
			short b = GetScriptValue();
			short c = GetScriptValue();

			mAngles.Set(a, b, c);
			break;
		}
		case 8487: {
			short a = GetScriptValue();
			short b = GetScriptValue();
			short c = GetScriptValue();

			mAngVel.Set(a, b, c);
			break;
		}
		case 8488: {
			short a = GetScriptValue();
			short b = GetScriptValue();
			short c = GetScriptValue();

			mAngAcc.Set(a, b, c);
			break;
		}
		default:
			CBaddy::SetVariable(variable);
			break;
	}
}
