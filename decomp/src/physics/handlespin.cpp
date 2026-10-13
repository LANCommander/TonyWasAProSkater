/* HandleSpin__6CBruce (0x800323e0) flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). Demo twin (fuzzy), locals from its MAIN.SYM. Called from DoPhysics right
   after FRUVectorsFromRotMatrix. Spinning and leaning: THPS1 replaces the demo's rotate-speed
   loop with L2/R2 tap turns (mTapTurn counts queued 180s; each frame rotates the heading and
   tallies the angle, scoring a 180 every 2048 units). Otherwise it picks the turn rate and the
   maximum lean from the board's turning stat (on the ground) or the shoulder buttons (in the
   air), leans toward the analogue target or the d-pad direction, decays the lean when nothing
   is pressed, then (inline here; a separate CalcAnimLeanFrame in the demo) picks the lean anim
   frame, and finally sets the pitch from up/down.
   Levers: every MaxLeanAngle assignment is a positive constant (THPS1 writes 0x5a000 where the
   demo doubled it), so combine's nonzero_bits proves it non-negative and (Max >> 12) / 2 becomes
   retail's single `srl 13`; the d-pad anim-lean clamp only stores when in range
   (if (a - d >= -Max) a -= d;), giving the two arms' shared `slt; bne; sw` tail; the anim-range
   tests are switches (signed slti trees); DeltaAngle's per-stat rate is one ?: expression. */
#include "game/bruce.h"
#include "game/m3dasm.h"
#include "game/vector.h"

extern SControl Pad[];
extern int XFrames;
extern int TimeScale;

int InterpolateFrame(int, int);

void CBruce::HandleSpin() {
	int DeltaAngle;
	int MaxLeanAngle;
	int LeftPressed;
	int RightPressed;

	if (mPhysicsState == 1 || mPhysicsState == 2) {
		if (!mNoSpin) {
			if (mpPad->TapSpinLeft.Triggered) {
				mpPad->TapSpinLeft.Triggered = 0;

				if (mTapTurn > 0)
					mTapTurn = 0;

				mTapTurn--;
			}

			if (mpPad->TapSpinRight.Triggered) {
				mpPad->TapSpinRight.Triggered = 0;

				if (mTapTurn < 0)
					mTapTurn = 0;

				mTapTurn++;
			}
		}

		if (mTapTurn > 0) {
			RotateHeadingAndVelocity((TimeScale * 150) >> 8, 0, 0);

			mTallyAngles += (TimeScale * 150) >> 8;

			if (mTallyAngles >= 2048) {
				mTallyAngles -= 2048;
				mTapTurn--;

				if (!mNoSpinScore)
					mNum180Spins++;
			}
		}

		if (mTapTurn < 0) {
			RotateHeadingAndVelocity(-((TimeScale * 150) >> 8), 0, 0);

			mTallyAngles -= (TimeScale * 150) >> 8;

			if (mTallyAngles <= -2048) {
				mTallyAngles += 2048;
				mTapTurn++;

				if (!mNoSpinScore)
					mNum180Spins++;
			}
		}
	} else
		mTapTurn = 0;

	if (mTapTurn) {
		mLeanAngle = 0;
		return;
	}

	mTurning = 0;

	if (mPhysicsState == 0) {
		DeltaAngle = (TimeScale * (mpStatBonus[4] == 1   ? 10240
		                           : mpStatBonus[4] == 3 ? 15872
		                                                 : 30720)) >>
		             8;
		MaxLeanAngle = 0x2d000;
		LeftPressed = mpPad->Left.Pressed;
		RightPressed = mpPad->Right.Pressed;
		mFirstLeftRightPressed = 0;

		if (mAY >= 31 || mpPad->Down.Pressed) {
			mDoingSharpTurn = 1;

			{
				int Speed = (M3dMaths_SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6) >> 12;

				DeltaAngle *= 2;
				MaxLeanAngle = 0x5a000;
				mAcc.vx -= mVel.vx / Speed;
				mAcc.vy -= mVel.vy / Speed;
				mAcc.vz -= mVel.vz / Speed;
			}
		}
	} else {
		if (Pad[0].SpinLeft.Pressed || Pad[0].SpinRight.Pressed)
			DeltaAngle = (TimeScale * 0x64000) >> 8;
		else
			DeltaAngle = (TimeScale * 0xa000) >> 8;

		MaxLeanAngle = 0xa0000;
		LeftPressed = mpPad->Left.Pressed || mpPad->SpinLeft.Pressed;
		RightPressed = mpPad->Right.Pressed || mpPad->SpinRight.Pressed;

		if (LeftPressed || RightPressed) {
			if (mFirstLeftRightPressed > 0 && XFrames - mFirstLeftRightPressed < 8) {
				LeftPressed = 0;
				RightPressed = 0;
			}
		} else
			mFirstLeftRightPressed = XFrames;

		LeftPressed |= mpPad->SpinLeft.Pressed;
		RightPressed |= mpPad->SpinRight.Pressed;

		if (mNoSpin)
			LeftPressed = RightPressed = 0;

		if (!LeftPressed && !RightPressed && !mAX)
			mLeanAngle = 0;

		mDoingSharpTurn = 0;
	}

	if (mAX && !LeftPressed && !RightPressed) {
		int TargetAngle = mAX * MaxLeanAngle / 128;

		if (TargetAngle < -MaxLeanAngle)
			TargetAngle = -MaxLeanAngle;

		if (TargetAngle > MaxLeanAngle)
			TargetAngle = MaxLeanAngle;

		if (mLeanAngle < TargetAngle) {
			int Diff = (TargetAngle - mLeanAngle) >> 12;

			mTurning = 1;
			mLeanAngle += DeltaAngle + DeltaAngle * Diff / ((MaxLeanAngle >> 12) / 2);

			if (mLeanAngle > TargetAngle)
				mLeanAngle = TargetAngle;

			mAnimLeanAngle += DeltaAngle;

			if (mAnimLeanAngle > TargetAngle)
				mAnimLeanAngle = TargetAngle;
		}

		if (mLeanAngle > TargetAngle) {
			int Diff = (mLeanAngle - TargetAngle) >> 12;

			mTurning = 1;
			mLeanAngle -= DeltaAngle + DeltaAngle * Diff / ((MaxLeanAngle >> 12) / 2);

			if (mLeanAngle < TargetAngle)
				mLeanAngle = TargetAngle;

			mAnimLeanAngle -= DeltaAngle;

			if (mAnimLeanAngle < TargetAngle)
				mAnimLeanAngle = TargetAngle;
		}
	} else if (LeftPressed) {
		mTurning = 1;
		mLeanAngle -= DeltaAngle;

		if (mLeanAngle < -MaxLeanAngle)
			mLeanAngle = -MaxLeanAngle;

		if (mAnimLeanAngle - DeltaAngle >= -MaxLeanAngle)
			mAnimLeanAngle -= DeltaAngle;
	} else if (RightPressed) {
		mTurning = 1;
		mLeanAngle += DeltaAngle;

		if (mLeanAngle > MaxLeanAngle)
			mLeanAngle = MaxLeanAngle;

		if (mAnimLeanAngle + DeltaAngle <= MaxLeanAngle)
			mAnimLeanAngle += DeltaAngle;
	} else {
		int Step;

		if (mDoingSharpTurn)
			mLeanAngle -= mLeanAngle >> 1;
		else
			mLeanAngle -= mLeanAngle >> 2;

		if (((mLeanAngle + 2048) >> 12) == 0)
			mLeanAngle = 0;

		Step = mAnimLeanAngle >> 4;

		if (Step < 0)
			if (Step > -1024)
				Step = -1024;

		if (Step > 0 && Step < 1024)
			Step = 1024;

		mAnimLeanAngle -= Step;

		if (((mAnimLeanAngle + 2048) >> 12) == 0)
			mAnimLeanAngle = 0;
	}

	mAnimLeanAngle = mLeanAngle;

	if (mAnimLeanAngle) {
		if (!mManIsCrouched) {
			switch (mAnim) {
				case 0:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
					if (mExtraFlags & 2) {
						if (mAnimLeanAngle < 0) {
							int Frame = -(mAnimLeanAngle * 22) / MaxLeanAngle;

							if (Frame > 22)
								Frame = 22;

							mAnimSpeed = 0x10000;

							if (mAnim != 7)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(7, mFrame, mFrame, 2881);
						} else {
							int Frame = mAnimLeanAngle * 22 / MaxLeanAngle;

							if (Frame > 22)
								Frame = 22;

							mAnimSpeed = 0x10000;

							if (mAnim != 6)
								mFrame = 0;

							if (mFrame < Frame)
								mFrame++;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(6, mFrame, mFrame, 2892);
						}
					} else {
						if (mAnimLeanAngle < 0) {
							int Frame = -(mAnimLeanAngle * 22) / MaxLeanAngle;

							if (Frame > 22)
								Frame = 22;

							mAnimSpeed = 0x10000;

							if (mAnim != 6)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(6, mFrame, mFrame, 2906);
						} else {
							int Frame = mAnimLeanAngle * 22 / MaxLeanAngle;

							if (Frame > 22)
								Frame = 22;

							mAnimSpeed = 0x10000;

							if (mAnim != 7)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(7, mFrame, mFrame, 2916);
						}
					}
			}
		}
	}

	if (mAnimLeanAngle) {
		if (mManIsCrouched) {
			switch (mAnim) {
				case 0:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
					if (mExtraFlags & 2) {
						if (mAnimLeanAngle < 0) {
							int Frame = -(mAnimLeanAngle * 12) / MaxLeanAngle;

							if (Frame > 12)
								Frame = 12;

							mAnimSpeed = 0x10000;

							if (mAnim == 7) {
								mAnim = 10;
								mFrame = Frame;
							}

							if (mAnim != 10)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(10, mFrame, mFrame, 2953);
						} else {
							int Frame = mAnimLeanAngle * 15 / MaxLeanAngle;

							if (Frame > 15)
								Frame = 15;

							mAnimSpeed = 0x10000;

							if (mAnim == 6) {
								mAnim = 9;
								mFrame = Frame;
							}

							if (mAnim != 9)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(9, mFrame, mFrame, 2968);
						}
					} else {
						if (mAnimLeanAngle < 0) {
							int Frame = -(mAnimLeanAngle * 15) / MaxLeanAngle;

							if (Frame > 15)
								Frame = 15;

							mAnimSpeed = 0x10000;

							if (mAnim == 6) {
								mAnim = 9;
								mFrame = Frame;
							}

							if (mAnim != 9)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(9, mFrame, mFrame, 2988);
						} else {
							int Frame = mAnimLeanAngle * 12 / MaxLeanAngle;

							if (Frame > 12)
								Frame = 12;

							mAnimSpeed = 0x10000;

							if (mAnim == 7) {
								mAnim = 10;
								mFrame = Frame;
							}

							if (mAnim != 10)
								mFrame = 0;

							mFrame = InterpolateFrame(mFrame, Frame);
							Debug_RunAnim2(10, mFrame, mFrame, 3003);
						}
					}
			}
		}
	}

	if (!mAnimLeanAngle) {
		switch (mAnim) {
			case 6:
			case 7:
				mAnimSpeed = 0x10000;
				Debug_RunAnim(0, 3017);
				break;
			case 9:
			case 10:
				mAnimSpeed = 0x10000;
				Debug_RunAnim3(8, 19, 26, 19, 3024);
				break;
		}
	}

	mPitchAngle = 0;

	if (mPhysicsState == 1 && !mpPad->Flip.Pressed && !mpPad->Grab.Pressed) {
		if (mpPad->Up.Pressed || mAY < -40)
			mPitchAngle = -((TimeScale * 20) >> 8);

		if (mpPad->Down.Pressed || mAY > 40)
			mPitchAngle = (TimeScale * 20) >> 8;
	}
}
