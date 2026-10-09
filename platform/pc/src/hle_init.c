/* Bring up the host devices before the game runs: the extracted data (from the disc on
 * first startup), window/GPU, input, sound. */
#include <SDL2/SDL.h>
#include <stdlib.h>

#include "pc_assets.h"
#include "pc_gpu.h"
#include "pc_hle.h"
#include "pc_runtime.h"
#include "pc_spu.h"

void PC_HleInit(void) {
    int scale = getenv("THPS_SCALE") ? atoi(getenv("THPS_SCALE")) : 3;
    SDL_SetMainReady();
    PC_AssetsInit();
    PC_GpuInit(scale > 0 ? scale : 3);
    PC_InputInit();
    PC_SpuInit();
}
