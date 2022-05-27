#include <stdio.h>
#include <stdlib.h>

/*
 * Definitions generated from Newton
 */
typedef double bmx055xAcceleration;

int
main (void)
{
	bmx055xAcceleration x1 = 3.2, x2 = 4.5;
	double y1, y2, y3, y4, y5, y6, y7, y8, y9;
	// add
	y1 = x1 + x2;
	y2 = x1 + 5.1;
	// sub
	y3 = x1 - x2;
	y4 = x1 - 3.7;
	y5 = 4.6 - x2;
	// multiply
	y6 = x1 * 3;
	// division
	y7 = x1 / 2;
	// shift left
	y8 = (int)x1 << 3;
	// shift right
	y9 = (int)x2 >> 4;
	return 0;
}
