/* Pack_Zap__Fiiii (0x8004a69c) */
typedef unsigned short ushort;
typedef unsigned char uchar;

struct TSPack {
	ushort X, Y, W, H;
	TSPack *pNext;
	char Type;
	uchar no_bottom, bpp, aligned;
};

TSPack *Pack_pFull;

TSPack *Pack_NewFree(int, int, int, int);
int Pack_Merge(TSPack *);

void Pack_Zap(int x, int y, int w, int h) {
	TSPack *p_loop;
REDO:
	p_loop = Pack_pFull;

	while (p_loop) {
		if (p_loop->X + (p_loop->W - 1) >= x && p_loop->X < x + w &&
		    p_loop->Y + (p_loop->H - 1) >= y && p_loop->Y < y + h)
			break;

		p_loop = p_loop->pNext;
	}

	if (p_loop == 0) {
		Pack_Merge(Pack_NewFree(x, y, w, h));
		return;
	}

	if (x >= p_loop->X && y >= p_loop->Y && p_loop->X + p_loop->W >= x + w &&
	    p_loop->Y + p_loop->H >= y + h)
		return;

	if (w < 32 && h < 32) {
		if (x < p_loop->X) {
			int half = p_loop->X - x;

			Pack_Zap(x, y, half, h);

			x += half;
			w -= half;
			goto REDO;
		}

		if (p_loop->X + p_loop->W < x + w) {
			int half = p_loop->X + p_loop->W - x;

			Pack_Zap(x, y, half, h);

			x += half;
			w -= half;
			goto REDO;
		}

		if (y < p_loop->Y) {
			int half = p_loop->Y - y;

			Pack_Zap(x, y, w, half);

			y += half;
			h -= half;
			goto REDO;
		}

		if (p_loop->Y + p_loop->H < y + h) {
			int half = p_loop->Y + p_loop->H - y;

			Pack_Zap(x, y, w, half);

			y += half;
			h -= half;
			goto REDO;
		}
	}

	if (w >= h) {
		int half = w / 2;

		Pack_Zap(x, y, half, h);

		x += half;
		w -= half;
	} else {
		int half = h / 2;

		Pack_Zap(x, y, w, half);

		y += half;
		h -= half;
	}

	goto REDO;
}
