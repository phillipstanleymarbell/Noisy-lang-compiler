#include <stdio.h>
#include <stdlib.h>

/*
 * Definitions generated from Newton
 */
typedef double bmx055xAcceleration; // [-16, 16]

static int ifStatement(bmx055xAcceleration x) {
	int y = 397;
	// if statement
	if (x > -34.19) {
		y += 10;
	} else {
		y /= 7;
	}

	if (x < 35.72) {
		y += 20;
	} else {
		y /= 11;
	}
	return y;
}

int
controlFlowFunc(double input)
{
	bmx055xAcceleration x;
    x = input;
	int y;

	y = ifStatement(x);

	return y;
}