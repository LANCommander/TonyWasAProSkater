/* Mods' skaters and levels and the career (pc_modcontent.cpp; see pc/mods.h).
 *
 * The career save (SCareer) has room for the game's 11 skaters and 9 levels only, and the indices
 * mods' skaters and levels get depend on which mods are loaded. Their progress (GCareerExt) is
 * kept beside the save, by name (<mod folder>/<manifest name>), in <name>.mods.ini; entries of
 * mods that aren't loaded are kept as they are. */
#ifndef PC_MODS_H
#define PC_MODS_H

#ifdef __cplusplus
extern "C" {
#endif

/* After the career save `name` was loaded into GCareer (and GCareerExt reset): read its mods'
 * progress, and put back (or clear) the skaters and levels GCareer names by index that this set
 * of mods doesn't have at that index. */
void PC_ModCareerLoad(const char *name);
/* After the career save `name` was written: write its mods' progress. */
void PC_ModCareerSave(const char *name);

#ifdef __cplusplus
}
#endif

#endif
