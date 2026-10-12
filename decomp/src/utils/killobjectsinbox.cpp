/* Utils_KillObjectsInBox__FRC7CVectorT0P5CBodyb (0x80048278) */
/* C++ (UTILS.CPP). Demo twin; locals from its MAIN.SYM. Kills every live body in the list whose
   position lies inside the box: Die() or a straight delete. Returns how many went. */
#include "game/ob.h"

int Utils_KillObjectsInBox(const CVector &min, const CVector &max, CBody *p_list, bool use_die) {
	int killed = 0;

	while (p_list) {
		CBody *p_next = (CBody *)p_list->mNextItem;

		if (!p_list->IsDead()) {
			CVector pos = p_list->mPos;

			if (pos.vx >= min.vx && max.vx >= pos.vx && pos.vz >= min.vz && max.vz >= pos.vz &&
			    pos.vy >= min.vy && max.vy >= pos.vy) {
				if (use_die)
					p_list->Die();
				else
					delete p_list;

				killed++;
			}
		}

		p_list = p_next;
	}

	return killed;
}
