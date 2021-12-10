#include <stdio.h>
#include <stdlib.h>

/*
 * Definitions generated from Newton
 */
typedef double signalAccelerationX;
typedef double signalAccelerationY;
typedef double signalAccelerationZ;

typedef struct complex {
    signalAccelerationX x;
    signalAccelerationY y;
} complex;

int
main (void)
{
	signalAccelerationX		accelerationX = 5.0;
	signalAccelerationY		accelerationY = 5.0;
    signalAccelerationX*	accelerationX_ptr = &accelerationX;
    double x;

    complex c;
    c.x = accelerationY;
    c.y = accelerationY;

	/*
	 * Dimensionally consistent statement
	 */
	*accelerationX_ptr = accelerationX + accelerationX;

	/*
	 * Dimensionally inconsistent statement
	 */
//	*accelerationX2 = accelerationX + accelerationY;
//    x = accelerationX;
//    x = accelerationY;

    printf("%f\n", accelerationX);
	return 0;
}
