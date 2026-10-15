/* SetVariable__6CBaddyUs (0x80040ccc) */
/* C++ (BADDY.CPP). Script "set" commands: each reads its operands from the script stream and
   stores them into the baddy (model, position, flags, attributes, registers, ...). */
#include "game/baddy.h"

int Spool_GetModel(uint, int);

void CBaddy::SetVariable(ushort variable) {
	uint *p_word;
	ushort which;

	switch (variable) {
		case 8501:
			p_word = (uint *)(((intptr_t)mpScript + 3) & -4);
			mDeadChecksum = *p_word;
			mpScript = (ushort *)(p_word + 1);
			break;
		case 8495:
			p_word = (uint *)(((intptr_t)mpScript + 3) & -4);
			mModel = Spool_GetModel(*p_word, mRegion);
			mpScript = (ushort *)(p_word + 1);
			break;
		case 8512:
			mPos.vx = (short)GetScriptValue() << 12;
			break;
		case 8513:
			mPos.vy = (short)GetScriptValue() << 12;
			break;
		case 8514:
			mPos.vz = (short)GetScriptValue() << 12;
			break;
		case 8497:
			if (*mpScript++)
				mFlags |= 8;
			else
				mFlags &= ~8;

			break;
		case 8494:
			mVerticalCompliance = *mpScript++;
			break;
		case 8492:
			mInputSignal = *(uchar *)mpScript;
			mpScript++;
			break;
		case 8493:
			mHit = *(uchar *)mpScript;
			mpScript++;
			break;
		case 8448:
			mHealth = *mpScript++;
			break;
		case 8449:
			mRMinor = *mpScript++;
			break;
		case 8485:
			which = *mpScript++;
			mBaddyAttributes[which] = *mpScript++;
			break;
		case 8468:
			mPattern = *mpScript++;
			break;
		case 8469:
			mSubPattern = *mpScript++;
			break;
		case 8480: {
			uint reg = *mpScript++;
			int value = GetScriptValue();

			mRegister[reg & 255] = value;
			break;
		}
		case 8481:
			mAnimSpeed = *(short *)mpScript++;
			break;
		case 8482:
			mGroundToOrigin = *mpScript++;
			break;
	}
}
