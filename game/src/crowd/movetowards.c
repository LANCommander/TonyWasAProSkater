/* Crowd_MoveTowards__FRiii (0x80010b58) */
int Crowd_MoveTowards(int *cur, int target, int speed) {
	if (*cur == target)
		return 0;

	if (*cur > target - speed && *cur < target + speed)
		*cur = target;
	else if (*cur < target)
		*cur += speed;
	else
		*cur -= speed;

	return 1;
}
