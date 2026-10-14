/* MToQ__FRC6MATRIXR5CQuat (0x800776f8) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/vector.h"

extern int DAT_800c110c[];

void MToQ(const MATRIX &m, CQuat &q) {
	int tr, s, i, j, k;

	tr = m.m[0][0] + m.m[1][1] + m.m[2][2];

	if (tr > 0) {
		s = ::SquareRoot0((tr + 4096) << 12);
		q.w = s >> 1;

		if (s)
			s = 0x800000 / s;

		q.x = ((m.m[1][2] - m.m[2][1]) * s) >> 12;
		q.y = ((m.m[2][0] - m.m[0][2]) * s) >> 12;
		q.z = ((m.m[0][1] - m.m[1][0]) * s) >> 12;
	} else {
		i = m.m[0][0] < m.m[1][1];

		if (m.m[i][i] < m.m[2][2])
			i = 2;

		j = DAT_800c110c[i];
		k = DAT_800c110c[j];
		s = ::SquareRoot0(((m.m[i][i] - (m.m[j][j] + m.m[k][k])) + 4096) << 12);

		switch (i) {
			case 0:
				q.x = s >> 1;
				break;
			case 1:
				q.y = s >> 1;
				break;
			case 2:
				q.z = s >> 1;
				break;
		}

		if (s)
			s = 0x800000 / s;

		q.w = ((m.m[j][k] - m.m[k][j]) * s) >> 12;

		switch (j) {
			case 0:
				q.x = ((m.m[i][j] + m.m[j][i]) * s) >> 12;
				break;
			case 1:
				q.y = ((m.m[i][j] + m.m[j][i]) * s) >> 12;
				break;
			case 2:
				q.z = ((m.m[i][j] + m.m[j][i]) * s) >> 12;
				break;
		}

		switch (k) {
			case 0:
				q.x = ((m.m[i][k] + m.m[k][i]) * s) >> 12;
				break;
			case 1:
				q.y = ((m.m[i][k] + m.m[k][i]) * s) >> 12;
				break;
			case 2:
				q.z = ((m.m[i][k] + m.m[k][i]) * s) >> 12;
				break;
		}
	}
}
