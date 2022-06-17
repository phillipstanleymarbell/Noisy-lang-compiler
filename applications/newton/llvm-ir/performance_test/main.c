//
// Created by ds123 on 2022/6/10.
//

/*
 * TODO
 * This file should be created automatically, including:
 * the specific header file,
 * the loop iteration numbers,
 * the api function that needed test.
 */
#include "../c-files/infer_bound_control_flow.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

int main() {
//	srand((unsigned)time(NULL));
//
//	struct timeval t1,t2;
//	double timeuse = 0, average_timeuse = 0;
//
//	// warm up
//	controlFlowFunc();

//	for (int idx = 0; idx < 100; idx++) {
//		gettimeofday(&t1,NULL);

		for (int i = 0; i < 1000000; i++) {
			controlFlowFunc();
		}

//		gettimeofday(&t2,NULL);
//		double t1_time = t1.tv_sec*1000000 + t1.tv_usec;
//		double t2_time = t2.tv_sec*1000000 + t2.tv_usec;
//		timeuse = t2_time - t1_time;
//		average_timeuse += timeuse;
//	}
//	average_timeuse /= 100;
//
//	printf("average time use: %f\n", average_timeuse);

	return 0;
}