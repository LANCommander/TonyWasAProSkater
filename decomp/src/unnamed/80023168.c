/* GameFMV_SetStartTrack__Fi (0x80023168) */
#include "names.h"
#ifdef PC_PORT
#include "pc_stream.h"
#endif

typedef struct {
	int w0;
	unsigned short h4, h6, h8, h10;
	int w12, w16;
	int b20;
	int w24, w28;
} E;

extern E movieDetails[];

int GameFMV_Active;
int CycleTracks;
unsigned char GameFMV_CurrentTrack;
unsigned short GameFMV_Width;
unsigned short GameFMV_Height;
int GameFMV_EndFrame;
int GameFMV_VLCBufferSize;
int GameFMV_RingBufferSectors;
unsigned char GameFMV_Is24Bit;
int GameFMV_MusicScale;
int GameFMV_StartSector;

void GameFMV_SetStartTrack(int a) {
	if (GameFMV_Active)
		return;

#ifdef PC_PORT
	/* the next song is the next entry of Data/Music/Playlist.m3u when there is one */
	if (a >= 0 && PC_MusicNextSong())
		return;
#endif

	if (a < 0) {
		a = ~a;
	} else {
		a = CycleTracks++;

		while (CycleTracks < 26) {
			if (movieDetails[CycleTracks].h4 == 96)
				break;

			CycleTracks++;
		}

		if (CycleTracks == 26)
			CycleTracks = 2;
	}

	GameFMV_CurrentTrack = a;
	GameFMV_Width = movieDetails[a].h4;
	GameFMV_Height = movieDetails[a].h6;
	GameFMV_EndFrame = movieDetails[a].h8;
	GameFMV_VLCBufferSize = movieDetails[a].w12;
	GameFMV_RingBufferSectors = movieDetails[a].w16;
	GameFMV_Is24Bit = movieDetails[a].b20;
	GameFMV_MusicScale = movieDetails[a].w28;
	GameFMV_StartSector = movieDetails[a].w24;
}
