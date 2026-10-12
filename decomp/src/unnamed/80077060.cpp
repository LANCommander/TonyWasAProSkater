/* FUN_80077060 (0x80077060) size=196 */
/* C++ (COMPETITION). The static teardown (_GLOBAL_.D, 0x80077060) and construction (_GLOBAL_.I,
   0x800770dc) of theCompetition (0x800c0fdc); one source covers both. CCompetition's destructor is the
   implicit one, which never re-stores its own vptr; only the inlined ~CClass store (0x800a3464)
   remains. The out-of-line copy is FUN_80077130. */
#include "game/competition.h"

CCompetition theCompetition;
