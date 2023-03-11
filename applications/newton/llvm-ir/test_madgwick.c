//
// Created by pei on 10/03/23.
//

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "c-files/MadgwickAHRS_softfloat.h"

#define DATA_SIZE 1000

/***************************************
 * Timer functions of the test framework
 ***************************************/

typedef struct timespec timespec;
timespec diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

timespec sum(timespec t1, timespec t2) {
    timespec temp;
    if (t1.tv_nsec + t2.tv_nsec >= 1000000000) {
        temp.tv_sec = t1.tv_sec + t2.tv_sec + 1;
        temp.tv_nsec = t1.tv_nsec + t2.tv_nsec - 1000000000;
    } else {
        temp.tv_sec = t1.tv_sec + t2.tv_sec;
        temp.tv_nsec = t1.tv_nsec + t2.tv_nsec;
    }
    return temp;
}

void printTimeSpec(timespec t, const char* prefix) {
    printf("%s: %d.%09d\n", prefix, (int)t.tv_sec, (int)t.tv_nsec);
}

timespec tic( )
{
    timespec start_time;
    clock_gettime(CLOCK_REALTIME, &start_time);
    return start_time;
}

timespec toc( timespec* start_time, const char* prefix )
{
    timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    timespec time_consump = diff( *start_time, current_time );
    printTimeSpec(time_consump, prefix );
    *start_time = current_time;
    return time_consump;
}

int main() {
    FILE* fp = fopen("input.csv", "r");

    if (!fp) {
        printf("Can't open file\n");
        return -1;
    }

    double time[DATA_SIZE];
    int32_t mag_x[DATA_SIZE], mag_y[DATA_SIZE], mag_z[DATA_SIZE],
            gyr_x[DATA_SIZE], gyr_y[DATA_SIZE], gyr_z[DATA_SIZE],
            acc_x[DATA_SIZE], acc_y[DATA_SIZE], acc_z[DATA_SIZE];

    char buffer[1024];
    int row = 0, column = 0;
    while (fgets(buffer, 1024, fp)) {
        column = 0;
        row++;

        if (row == 1)
            continue;

        char* value = strtok(buffer, ", ");

        while (value) {
            switch (column) {
                case 0:
                    time[row-2] = atof(value);
                    break;
                case 1:
                    mag_x[row-2] = atoi(value);
                    break;
                case 2:
                    mag_y[row-2] = atoi(value);
                    break;
                case 3:
                    mag_z[row-2] = atoi(value);
                    break;
                case 4:
                    gyr_x[row-2] = atoi(value);
                    break;
                case 5:
                    gyr_y[row-2] = atoi(value);
                    break;
                case 6:
                    gyr_z[row-2] = atoi(value);
                    break;
                case 7:
                    acc_x[row-2] = atoi(value);
                    break;
                case 8:
                    acc_y[row-2] = atoi(value);
                    break;
                case 9:
                    acc_z[row-2] = atoi(value);
                    break;
                default:
                    break;
            }

            value = strtok(NULL, ", ");
            column++;
        }
    }

    fclose(fp);

    timespec timer = tic();
    for (size_t ts = 0; ts < DATA_SIZE; ts++) {
        MadgwickAHRSupdate(gyr_x[ts], gyr_y[ts], gyr_z[ts],
                           acc_x[ts], acc_y[ts], acc_z[ts],
                           mag_x[ts], mag_y[ts], mag_z[ts]);
    }
    toc(&timer, "computation delay");
    return 0;
}
