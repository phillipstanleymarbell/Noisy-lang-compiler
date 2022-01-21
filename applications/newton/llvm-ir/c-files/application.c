#include <stdio.h>
#include <stdlib.h>

/*
 * Definitions generated from Newton
 */
typedef double signalAccelerationX;
typedef double signalAccelerationY;
typedef double signalAccelerationZ;

typedef struct complex {
    signalAccelerationX	x;
    signalAccelerationY	y;
} complex;

int
main (void)
{
	signalAccelerationX		accelerationX = 5.0;
	signalAccelerationY		accelerationY = 5.0;
    signalAccelerationX *	accelerationX_ptr = &accelerationX;
    double x;

    complex c;
    c.x = accelerationX;
    c.y = accelerationY;

	/*
	 * Dimensionally consistent statements
	 */
	*accelerationX_ptr = accelerationX + accelerationX;
	x = accelerationX;

	/*
	 * Dimensionally inconsistent statements
	 *
	 * accelerationX = accelerationX || accelerationY;
	 * accelerationX2 = accelerationX + accelerationY;
	 *
	 * Since x has already been assigned previously with a signalAccelerationX signal,
	 * the following instruction is invalid:
	 *
	 * x = accelerationY;
	 *
	 * c.y = accelerationX;
	 */

    printf("%f\n", accelerationX);
	return 0;
}
