#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

/*
 * Definitions generated from Newton
 */
typedef double bmx055xAcceleration; // [-16, 16]

// random floating point, [min, max]
float randomFloat(float min, float max) {
	float referenceValue = min + 1.0 * rand() / RAND_MAX * (max - min);
//	printf("reference value: %f\n", referenceValue);
	return referenceValue;
}

int ifStatement(bmx055xAcceleration x, float referenceValue) {
	int y;
	// if statement
	if (x > referenceValue)
	{
		y = 0;
	}
	else
	{
		y = 10;
	}
	// so y1=0
	return y;
}

//int elseStatement(bmx055xAcceleration x, float referenceValue) {
//	int y;
//	// else statement
//	if (x < referenceValue)
//	{
//		y = 0;
//	}
//	else
//	{
//		y = 10;
//	}
//	// so y2=10
//	return y;
//}

int
main (void)
{
	bmx055xAcceleration x = randomFloat(-16, 16);
	int y1, y2, y3;
	int y4 = 4;
	int y5 = 5;
	int y6 = 6;
	int y7 = 7;
	int y8 = 8;
	int y9 = 9;
	int y10 = 10;
	int y11 = 11;

	srand((unsigned)time(NULL));

	struct timeval t1,t2;
	double timeuse;
	gettimeofday(&t1,NULL);

	for (size_t i = 0; i < 10000; i++) {
		// basic
		y1 = ifStatement(x, randomFloat(-50, -30));

//		y2 = elseStatement(x);
	}

	gettimeofday(&t2,NULL);
	double t1_time = t1.tv_sec*1000000 + t1.tv_usec;
	double t2_time = t2.tv_sec*1000000 + t2.tv_usec;
	timeuse = t2_time - t1_time;
	printf("y1: %d\ttime use: %f\n", y1, timeuse);

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