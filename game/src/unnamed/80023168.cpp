/* GameFMV_SetStartTrack__Fi (0x80023168) */
#include "game/gamefmv.h"
#include "pc_stream.h"

extern SMovieDetails movieDetails[];

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

	/* the next song is the next entry of Data/Music/Playlist.m3u when there is one */
	if (a >= 0 && PC_MusicNextSong())
		return;

	if (a < 0) {
		a = ~a;
	} else {
		a = CycleTracks++;

		while (CycleTracks < 26) {
			if (movieDetails[CycleTracks].width == 96)
				break;

			CycleTracks++;
		}

		if (CycleTracks == 26)
			CycleTracks = 2;
	}

	GameFMV_CurrentTrack = a;
	GameFMV_Width = movieDetails[a].width;
	GameFMV_Height = movieDetails[a].height;
	GameFMV_EndFrame = movieDetails[a].endframe;
	GameFMV_VLCBufferSize = movieDetails[a].vlcbuffersize;
	GameFMV_RingBufferSectors = movieDetails[a].ringbuffersectors;
	GameFMV_Is24Bit = movieDetails[a].is24bit;
	GameFMV_MusicScale = movieDetails[a].musicScale;
	GameFMV_StartSector = movieDetails[a].startsector;
}
