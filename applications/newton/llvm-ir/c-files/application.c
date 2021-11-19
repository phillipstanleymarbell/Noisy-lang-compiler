#include <stdio.h>
#include <stdlib.h>

/*
 * Definitions generated from Newton
 */
typedef double signalAccelerationX;
typedef double signalAccelerationY;
typedef double signalAccelerationZ;

int
main (void)
{
	signalAccelerationX		accelerationX = 5.0, accelerationX2;
	signalAccelerationY		accelerationY = 5.0;
	signalAccelerationZ		accelerationZ = 5.0;
    double x;

	/*
	 * Dimensionally consistent statement
	 */
	accelerationX2 = accelerationX + accelerationX;

	/*
	 * Dimensionally inconsistent statement
	 */
	accelerationX2 = accelerationX + accelerationX;
//    x = accelerationX2;
	return 0;
}
