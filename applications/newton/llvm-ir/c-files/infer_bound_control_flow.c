#include <stdio.h>
#include <stdlib.h>

/*
 * Definitions generated from Newton
 */
typedef double bmx055xAcceleration;

int
main (void)
{
	bmx055xAcceleration x;
	int y1, y2, y3;
	int y4 = 4;
	int y5 = 5;
	int y6 = 6;
	int y7 = 7;
	int y8 = 8;
	int y9 = 9;
	int y10 = 10;
	int y11 = 11;

	// basic

	// if statement
	if (x > -100)
	{
		y1 = 0;
	}
	else
	{
		y1 = 10;
	}
	// so y1=0

	if (x < -20)
	{
		y2 = 0;
	}
	else
	{
		y2 = 10;
	}
	// so y2=10

//	// for statement
//	for (int i = 20; i < x; i++)
//	{
//		// this for block should be eliminated
//		y4++;
//	}
//
//	for (int i = -20; i > x; i--)
//	{
//		// this for block should be eliminated
//		y5++;
//	}
//
//	// while statement
//	while (x > 20)
//	{
//	  // this for block should be eliminated
//	  y8++;
//	}
//
//	while (x < -20)
//	{
//	  // this for block should be eliminated
//	  y9++;
//	}
//
//	// enhancement
//
//	if (x > 0)
//	{
//	  y3 = 0;
//	}
//	else
//	{
//	  y3 = 10;
//	}
//	// so y3={0,10}
//
//	// increasing
//	// if x in [-16, 2], this for block should be eliminated
//	// if x in [3, 16], y6=y6+(x-i0)
//	// so the range of y6 is [y6, y6+x-2], where x>2
//	for (int i = 2; i < x; i++)
//	{
//		// it's better to infer the range of y6, which is [6, 20]
//		y6++;
//	}
//
//	// decreasing
//	// if x in [1, 16], this for block should be eliminated
//	// if x in [-16, 0], y7 = y7+(i0-x)
//	// so the range of y7 is [y7, y7+1-x], where x<1
//	for (int i = 1; i > x; i--)
//	{
//		// it's better to infer the range of y7, which is [7, 24]
//		y7++;
//	}
//
//	// increasing
//	// if x in [5, 16], this for block should be eliminated
//	// if x in [-16, 4], y10=y10+(5-x)
//	// so the range of y6 is [y10, y10+5-x], where x<5
//	while (x < 5)
//	{
//		// it's better to infer the range of y10, which is [10, 31]
//		y10++;
//		x++;
//	}
//
//	// decreasing
//	// if x in [-16, 0], this for block should be eliminated
//	// if x in [1, 16], y11=y11+(x-0)
//	// so the range of y11 is [y11, y11+x-0], where x>0
//	while (x > 0)
//	{
//		// it's better to infer the range of y11, which is [11, 27]
//		y11++;
//		x--;
//	}

	return 0;
}