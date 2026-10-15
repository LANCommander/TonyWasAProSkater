/* Saves on PC (src/pc_saves.cpp; the game's interface is pc_saves.h): a file per career and per
 * replay in the player's folder, PS1 .mcs saves imported from the Saves folder. */
#pragma once

#include <stdint.h>

#include "pc/platform.h"
#include "pc_saves.h"

namespace pc {

class Saves {
  public:
    /* The game's save directory table (SLOT_DIR, 15 libapi DIRENTRYs; FileType, one EFileType per
     * entry): the saves of one type, newest first. Returns the number listed. */
    int ListDir(int type, void *slotDir, int32_t *fileTypes);
    /* The type of the last ListDir. */
    int ListType() const { return m_listType; }

    /* The save a SLOT_DIR entry names (Card_SetSelectedFile): the one Card_Load reads. */
    void Select(const char *name);
    const char *Selected() const { return m_selected; }

    /* A card header (SCardHead, 512 bytes) with the name as the Shift-JIS title Card_Write would
     * have written for that type. */
    static void CardHeader(void *head, int type, const char *name);

    bool Exists(int type, const char *name);
    /* The file is replaced whole (through a temporary file). */
    bool Write(int type, const char *name, int n, const PCSaveChunk *chunks);
    /* Fills the chunks from the start of the payload: false if the file is missing, isn't a save
     * of that type, or is shorter than the chunks. */
    bool Read(int type, const char *name, int n, const PCSaveChunk *chunks);

  private:
    const char *RootDir();
    const char *TypeDir(int type);
    bool SavePath(int type, const char *name, char *out, int n);
    void ImportMcs(const char *path);
    void ImportPs1Saves();

    int m_listType = 0;
    char m_selected[PC_SAVE_NAME_LEN + 1] = {};
    char m_rootDir[MAX_PATH] = {};      /* RootDir, once made */
    char m_typeDirs[2][MAX_PATH] = {};  /* TypeDir, once made */
    bool m_imported = false;            /* the .mcs files were looked for */
};

extern Saves g_saves;

}   // namespace pc
