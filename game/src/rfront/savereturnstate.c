/* SaveReturnState__Fv (0x80012774) */
#include "names.h"

extern int GSaveAction;
extern int GReturnToState;

int InCompetition(void);
int Competition_IsOver(void);

int SaveReturnState(void) {
	if (GSaveAction == 0) {
		if (InCompetition() && !Competition_IsOver())
			return 25;

		return 15;
	}

	return GReturnToState;
}
