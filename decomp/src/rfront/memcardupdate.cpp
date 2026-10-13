/* Front_MemCardUpdate__Fv (0x800127c4) */
/* C++ (RFRONT.CPP), from the demo twin + its MAIN.SYM locals (Activated, GoBack, AnyButton, Start;
   block locals UpDown, FileType, WriteError, save_name, WantedFileType/Vids/m/CardHead/p, pTmp).
   The memory-card front-end state machine; returns the next front-end screen (18 = stay).
   Levers: case bodies of the CardStatus sub-switches in demo source order (case 1 first); the
   string table pointers are fields of the front-end string struct GameText (lui of the struct
   first); the SaveFile/LoadFile name copy is an if/else of two calls; `if (!WriteError)` first; the
   video-header copy in case 1 reads NumFrames/Highlights through VH; "demoX.rec" is a 10-byte
   word-aligned block copy (strcpy of a literal), done as memcpy from the aligned extern. */
#include "game/card.h"
#include "game/career.h"
#include "game/video.h"
#include "game/pad.h"
#include "game/front.h"
#include "game/shellutils.h"

extern "C" {
	extern int TTime;
	extern int CardStatus;
	extern CMenu *pYesNoMenu;
	extern int GNumberOfPlayers;
	extern int GSoftResetEnabled;
	extern int VblankUpdateBar;
	extern SControl Pad[2];
	extern SVideoInfo GVInfo1;
	extern SVideoRestart GVideoRestart;
	extern SCareer GCareer;
	extern unsigned char SaveGameName[];
	extern char s_800a42dc[];
	extern char s_800a42e8[];
	extern char StoredCareerName[];
	extern char ASCIIName[][11];
	extern char *VertMenuItem[];
	extern int VertMenuItemIndex[];
	extern int FileType[];
	extern char SLOT_DIR[][40];
	extern LocalText GameText;

	void *memcpy(void *, const void *, unsigned int);
	void FUN_800117b4(void);
	int FUN_80012600(void);
	void FUN_80012650(void);
	int FUN_800144a8(int);
	int Card_Write__F9EFileTypei(int, int);
	int FUN_80014868(int, char *);
	int SFX_PlayX__FUisi(unsigned int, short, int);
	void *Mem_NewX__FUiii(unsigned int, int, int);
	void Mem_Delete__FPv(void *);
	int FUN_8008c190(char *, ...);
	char *FUN_8008e904(char *, char *);
}

#ifdef PC_PORT
#include "pc_saves.h"
#endif

VerticalMenu *pVertMenu;
int VideoToDisk, GHasCareerLoaded, GLoadedCareerIsSaved, CursorPos, SizeUp;
int NumVertMenuItems, VertMenuTopIndex, VertMenuTopChanged, MCState, CheckingCardPause;
int GLoadAction, GSaveAction;
int Delay, Retries, SaveNeedBlocks, SaveFreeBlocks, GReturnToState;

int Card_CheckStatus(int, int);
int Card_GetDirAndFreeBlocks(int, int);
int Card_FormatCard(int, int);
int Card_ReadFileHeader(char *, SCardHead *);
void Card_SetSelectedFile(char *);
int Card_Load(EFileType, ulong *, ulong *);
void Front_ExtractASCIINameFromSJIS(char *, char *);
void Front_GetButtons(int &, int &, int &, int &);
int GetUpDown();
int SaveReturnState();

int Front_MemCardUpdate() {
	int Activated;
	int GoBack;
	int AnyButton;
	int Start;

	if (++TTime & 1)
		Card_CheckStatus(0, 0);

	Front_GetButtons(Activated, GoBack, AnyButton, Start);

	switch (MCState) {
		case 0:
#ifdef PC_PORT
			/* PC: no card to check (the saves are files, pc_saves.h) */
			MCState = 1;
			break;
#endif
			if (CheckingCardPause) {
				if (--CheckingCardPause == 0)
					MCState = 1;
			} else {
				CheckingCardPause = 10;
			}

			break;
		case 1:
			if (VideoToDisk) {
				MCState = 2;
				break;
			}

			switch (CardStatus) {
				case 1:
#ifdef PC_PORT
					/* PC: the saves of the type being saved (FUN_8001571c looks through the
					   careers); there's always room */
					PC_SaveListDir(GSaveAction == 1 ? PC_SAVE_CAREER : PC_SAVE_REPLAY, SLOT_DIR,
					               (int32_t *)FileType);

					SaveFreeBlocks = 15;
#else
					SaveFreeBlocks = Card_GetDirAndFreeBlocks(0, 0);
#endif

					if (GSaveAction == 1) {
						SaveNeedBlocks = 1;

						if (SaveFreeBlocks < 1 && !GHasCareerLoaded) {
							MCState = 8;
							break;
						}

						if (GHasCareerLoaded) {
							{
								int i;

								for (i = 0; i < 10; i++)
									SaveGameName[i] = StoredCareerName[i];
							}

							MCState = 4;
							Delay = 4;
							Retries = 4;
						} else {
							MCState = 2;
							FUN_800117b4();
						}
					} else {
						SaveNeedBlocks = 3;

						if (GNumberOfPlayers == 2)
							SaveNeedBlocks = 6;

						if (SaveFreeBlocks < SaveNeedBlocks) {
							MCState = 8;
						} else {
							MCState = 2;
							FUN_800117b4();
						}
					}

					break;
				case -1:
					MCState = 9;
					break;
				case -2:
					MCState = 10;
					pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText.pNo));
					break;
			}

			break;
		case 8:
			switch (CardStatus) {
				case -1:
					MCState = 9;
					break;
				case -2:
				case 2:
					MCState = 0;
					break;
			}

			if (AnyButton) {
				SFX_PlayX__FUisi(2, 0x2000, 0);
				return SaveReturnState();
			}

			break;
		case 2: {
			int UpDown = GetUpDown();

			if (UpDown == -1) {
				SizeUp = 16;
				SFX_PlayX__FUisi(17, 0x2000, 0);

				if (SaveGameName[CursorPos] == ' ' || SaveGameName[CursorPos] == 0) {
					SaveGameName[CursorPos] = 'A';
				} else {
					SaveGameName[CursorPos]++;

					if (SaveGameName[CursorPos] >= 91)
						SaveGameName[CursorPos] = ' ';
				}
			}

			if (UpDown == 1) {
				SizeUp = 16;
				SFX_PlayX__FUisi(17, 0x2000, 0);

				if (SaveGameName[CursorPos] == ' ' || SaveGameName[CursorPos] == 0) {
					SaveGameName[CursorPos] = 'Z';
				} else {
					SaveGameName[CursorPos]--;

					if (SaveGameName[CursorPos] < 65)
						SaveGameName[CursorPos] = ' ';
				}
			}

			if (Pad[0].MenuLeft.Triggered || Pad[1].MenuLeft.Triggered) {
				SizeUp = 16;
				SFX_PlayX__FUisi(5, 0x2000, 0);

				Pad[0].MenuLeft.Triggered = 0;
				Pad[1].MenuLeft.Triggered = 0;
				CursorPos--;

				if (CursorPos < 0)
					CursorPos = 9;
			}

			if (Pad[0].MenuRight.Triggered || Pad[1].MenuRight.Triggered) {
				SizeUp = 16;
				SFX_PlayX__FUisi(5, 0x2000, 0);

				Pad[0].MenuRight.Triggered = 0;
				Pad[1].MenuRight.Triggered = 0;
				CursorPos++;

				if (CursorPos >= 10)
					CursorPos = 0;
			}

			if (GSaveAction == 0)
				FUN_8008e904(s_800a42e8, (char *)SaveGameName);
			else
				FUN_8008e904(s_800a42dc, (char *)SaveGameName);

			if (GoBack) {
				SFX_PlayX__FUisi(2, 0x2000, 0);
				return SaveReturnState();
			}

			if (CardStatus == -1 && !VideoToDisk)
				MCState = 9;

			if (Activated && !VideoToDisk) {
				SFX_PlayX__FUisi(5, 0x2000, 0);

				{
					int FileType = GSaveAction == 0;

					if (FUN_80012600()) {
						do
							FUN_80012650();

						while (FUN_800144a8(FileType));
					}

					if (FUN_800144a8(FileType) && !(GSaveAction == 1 && GHasCareerLoaded)) {
						MCState = 3;
						pYesNoMenu->SetLine(pYesNoMenu->FindEntry(GameText.pNo));
						pYesNoMenu->CentreY();
					} else {
						MCState = 4;
						Delay = 4;
						Retries = 4;
					}
				}
			}

			if (Activated && VideoToDisk) {
				SFX_PlayX__FUisi(5, 0x2000, 0);

				MCState = 5;
			}
		} break;
		case 3:
			pYesNoMenu->Update();

			if (Activated) {
				if (pYesNoMenu->ChoiceIs(GameText.pYes)) {
					SFX_PlayX__FUisi(5, 0x2000, 0);

					MCState = 4;
					Delay = 4;
					Retries = 4;
				} else {
					SFX_PlayX__FUisi(2, 0x2000, 0);

					MCState = 2;
					FUN_800117b4();
				}
			}

			if (GoBack) {
				SFX_PlayX__FUisi(2, 0x2000, 0);

				MCState = 2;
				FUN_800117b4();
			}

			break;
		case 9:
			switch (CardStatus) {
				case -2:
				case 1:
				case 2:
					MCState = 0;
					break;
			}

			if (AnyButton) {
				SFX_PlayX__FUisi(2, 0x2000, 0);
				return SaveReturnState();
			}

			break;
		case 18:
			if (CardStatus == -1)
				MCState = 17;

			if (AnyButton) {
				SFX_PlayX__FUisi(GoBack ? 147 : 145, 0x2000, 0);
				return SaveReturnState();
			}

			break;
		case 10:
			pYesNoMenu->Update();

			switch (CardStatus) {
				case 1:
				case 2:
					MCState = 0;
					break;
				case -1:
					MCState = 9;
					break;
			}

			if (Activated) {
				if (pYesNoMenu->ChoiceIs(GameText.pYes)) {
					SFX_PlayX__FUisi(5, 0x2000, 0);

					MCState = 11;
					Delay = 4;
					Retries = 4;
				} else {
					SFX_PlayX__FUisi(5, 0x2000, 0);
					return SaveReturnState();
				}
			}

			if (GoBack) {
				SFX_PlayX__FUisi(2, 0x2000, 0);

				MCState = 0;
			}

			break;
		case 11:
			GSoftResetEnabled = 0;

			if (CardStatus == 2 || CardStatus == -1) {
				MCState = 12;
				Delay = 75;
				break;
			}

			if (--Delay < 0 && CardStatus) {
				if (Card_FormatCard(0, 0) == 1) {
					MCState = 13;
					Delay = 75;
				} else if (--Retries < 0) {
					MCState = 12;
					Delay = 75;
				} else {
					Delay = (4 - Retries) * 60;
				}
			}

			break;
		case 6:
		case 7:
			if (Activated) {
				SFX_PlayX__FUisi(5, 0x2000, 0);
				return SaveReturnState();
			}

			break;
		case 12:
		case 13:
			if (--Delay < 0) {
				if (CardStatus == -1)
					MCState = 9;
				else
					MCState = 0;
			}

			if (AnyButton) {
				SFX_PlayX__FUisi(GoBack ? 2 : 5, 0x2000, 0);

				MCState = 0;
			}

			break;
		case 4:
			GSoftResetEnabled = 0;

			if (CardStatus == -1) {
				MCState = 6;
				Delay = 75;
			}

			if (--Delay < 0 && CardStatus) {
				int WriteError;

				switch (GSaveAction) {
					case 0:
						SaveNeedBlocks = 3;

						if (GNumberOfPlayers == 2)
							SaveNeedBlocks = 6;

						WriteError = Card_Write__F9EFileTypei(1, SaveNeedBlocks);
						break;
					case 1:
						SaveNeedBlocks = 1;
						WriteError = Card_Write__F9EFileTypei(0, SaveNeedBlocks);
						break;
				}

				if (!WriteError) {
					MCState = 7;

					if (GSaveAction == 1) {
						GHasCareerLoaded = 1;
						GLoadedCareerIsSaved = 1;

						{
							int i;

							for (i = 0; i < 10; i++)
								StoredCareerName[i] = SaveGameName[i];
						}
					}
				} else {
					MCState = 6;
					Delay = 75;
				}
			}

			break;
		case 5:
			GSoftResetEnabled = 0;

			if (SaveGameName[0] < 'A' || SaveGameName[0] > 'Z')
				SaveGameName[0] = 'A';

			if (SaveGameName[0] >= 'K') {
				MCState = 2;
				Activated = 0;
				GoBack = 0;
				break;
			}

			{
				char save_name[16];

				memcpy(save_name, "demoX.rec", 10);

				save_name[4] = SaveGameName[0];
				FUN_8008c190("Saving Replay %s", save_name);
				FUN_80014868(1, save_name);

				MCState = 7;
			}

			break;
		case 14:
			if (AnyButton)
				SFX_PlayX__FUisi(5, 0x2000, 0);

			break;
		case 15:
#ifdef PC_PORT
			MCState = 16;
			break;
#endif
			if (CheckingCardPause) {
				if (--CheckingCardPause == 0)
					MCState = 16;
			} else {
				CheckingCardPause = 10;
			}

			break;
		case 16:
			switch (CardStatus) {
				case 1: {
					int WantedFileType;
					int Vids;
					int Offset;

#ifndef PC_PORT
					SaveFreeBlocks = Card_GetDirAndFreeBlocks(0, 0);
#endif
					WantedFileType = 1;
					Offset = 26;

					if (GLoadAction == 2) {
						WantedFileType = 0;
						Offset = 28;
					}

#ifdef PC_PORT
					/* PC: only the saves of the type being loaded, so there can be 15 of each */
					PC_SaveListDir(WantedFileType, SLOT_DIR, (int32_t *)FileType);

					SaveFreeBlocks = 15;
#endif

					Vids = 0;

					{
						int i;

						for (i = 0; i < 15; i++)
							Vids += FileType[i] == WantedFileType;
					}

					if (Vids) {
						int m = 0;

						{
							int i;

							for (i = 0; i < 15; i++) {
								if (FileType[i] == WantedFileType) {
									SCardHead CardHead;
									char *p;

									Card_ReadFileHeader(SLOT_DIR[i], &CardHead);

									p = ASCIIName[m];

									{
										int j;

										for (j = 0; j < 10; j++) {
											if (!CardHead.Title[Offset + j * 2]) {
												*p++ = 0;
												break;
											}

											if ((uchar)CardHead.Title[Offset + j * 2] == 0x82)
												*p++ =
												    (uchar)CardHead.Title[Offset + j * 2 + 1] - 31;
											else
												*p++ = ' ';
										}
									}

									*p = 0;
									VertMenuItem[m] = ASCIIName[m];
									VertMenuItemIndex[m] = i;
									m++;
								}
							}
						}

						NumVertMenuItems = m;
						VertMenuTopIndex = 0;
						MCState = 20;
					} else {
						MCState = 19;
					}
				} break;
				case -1:
					MCState = 17;
					break;
				case -2:
					MCState = 18;
					break;
			}

			break;
		case 17:
			switch (CardStatus) {
				case -2:
				case 1:
				case 2:
					MCState = 15;
					break;
			}
		case 19:
			if (AnyButton) {
				SFX_PlayX__FUisi(GoBack ? 147 : 145, 0x2000, 0);
				return SaveReturnState();
			}

			break;
		case 20:
			VertMenuTopChanged = 0;

			if (Pad[0].MenuUp.Triggered || Pad[1].MenuUp.Triggered) {
				Pad[0].MenuUp.Triggered = 0;
				Pad[1].MenuUp.Triggered = 0;

				if (pVertMenu->getSelected() > 0) {
					pVertMenu->up();
				} else if (VertMenuTopIndex > 0) {
					VertMenuTopIndex--;
					VertMenuTopChanged = 1;
				}
			}

			if (Pad[0].MenuDown.Triggered || Pad[1].MenuDown.Triggered) {
				Pad[0].MenuDown.Triggered = 0;
				Pad[1].MenuDown.Triggered = 0;

				if (VertMenuTopIndex < NumVertMenuItems - 5) {
					VertMenuTopIndex++;
					VertMenuTopChanged = 1;
				} else if (pVertMenu->getSelected() < 4) {
					pVertMenu->down();
				}
			}

			if (GoBack || CardStatus == -1) {
				SFX_PlayX__FUisi(147, 0x2000, 0);
				return GReturnToState;
			}

			if (Activated) {
				Card_SetSelectedFile(
				    SLOT_DIR[VertMenuItemIndex[VertMenuTopIndex + pVertMenu->getSelected()]]);
				SFX_PlayX__FUisi(145, 0x2000, 0);

				MCState = 21;
			}

			break;
		case 21:
			GSoftResetEnabled = 0;

			switch (GLoadAction) {
				case 0: {
					int Error;

					VblankUpdateBar = 1;
					Error = Card_Load(THPS_VIDEO_FILE, GVInfo1.pBlockStart, 0);
					VblankUpdateBar = 0;

					if (Error) {
						MCState = 22;
					} else {
						int i;
						SVideoHead *VH = (SVideoHead *)(GVInfo1.pBlockStart + 128);

						GVInfo1.NumFrames = VH->NumFrames;

						for (i = 0; i < 5; i++)
							GVInfo1.Highlights[i] = VH->Highlights[i];

						MCState = 23;
					}
				} break;
				case 1:
					if (Card_Load(THPS_VIDEO_HEADER, GVInfo1.pBlockStart, 0)) {
						MCState = 22;
					} else {
						{
							SVideoHead *VH = (SVideoHead *)(GVInfo1.pBlockStart + 128);

							GVideoRestart.NumSkaters = VH->NumSkaters;
							GVideoRestart.Skater = VH->Skater;
							GVideoRestart.Skater2 = VH->Skater2;
							GVideoRestart.Level = VH->Level;
							GVideoRestart.GGame = VH->GGame;

							{
								int i;

								GVInfo1.NumFrames = VH->NumFrames;

								for (i = 0; i < 5; i++)
									GVInfo1.Highlights[i] = VH->Highlights[i];
							}
						}

						MCState = 23;
					}

					break;
				case 2: {
					ulong *pTmp = (ulong *)Mem_NewX__FUiii(3200, 1, 1);

					if (Card_Load(THPS_SAVE_FILE, pTmp, 0)) {
						MCState = 22;
					} else {
						GCareer = *(SCareer *)(pTmp + 160);
						GHasCareerLoaded = 1;
						GLoadedCareerIsSaved = 0;
						Front_ExtractASCIINameFromSJIS((char *)pTmp + 32, StoredCareerName);

						MCState = 23;
					}

					Mem_Delete__FPv(pTmp);
				} break;
			}

			break;
		case 22:
			if (AnyButton) {
				SFX_PlayX__FUisi(GoBack ? 147 : 145, 0x2000, 0);
				return 36;
			}

			break;
		case 23:
			return 35;
		case 27:
			break;
	}

	return 18;
}
