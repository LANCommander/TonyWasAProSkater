/* removeFromMemory__11SlicedImage (0x80062de8) */
/* C++ (SHELLUTILS.CPP). Frees the image's pixel copy in main RAM (once). */
#include "game/shellutils.h"

void Mem_Delete(void *);

void SlicedImage::removeFromMemory() {
	if (!m_removed_from_memory) {
		m_removed_from_memory = 1;
		Mem_Delete(slicedBuffer);
	}
}
