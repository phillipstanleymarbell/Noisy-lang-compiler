/*
 * Madgwick test case (run locally)
 * Compilation command in applications/newton/llvm-ir/performance_test/Makefile
 *
 * How to compile and run?
 * 1. `make perf_madgwick` FP hardware (by default)
 * 2. `SOFT_FLOAT=1 make perf_madgwick` FP software (clang -msoft-float and opt --float-abi=soft)
 * 3. `SOFT_FLOAT_LIB=1 make perf_madgwick` FP soft-float lib (from CHStone)
 * 4. `AUTO_QUANT=1 make perf_madgwick` INT fixed Q number format
 * */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#if defined(INT_DATA_TYPE)
extern volatile int32_t	q0, q1, q2, q3;
#include "c-files/MadgwickAHRSfix.h"
#elif defined(FP_DATA_TYPE)
extern volatile float	q0, q1, q2, q3;
#if defined(SOFT_FLOAT_LIB)
#include "c-files/MadgwickAHRS_softfloat.h"
#else
#include "c-files/MadgwickAHRS.h"
#endif

#else
#error "Must set data type: FP or INT"
#endif

#define DATA_SIZE 1000

#define ITERATION 10

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
#if defined(INT_DATA_TYPE)
    int32_t	q0[DATA_SIZE], q1[DATA_SIZE], q2[DATA_SIZE], q3[DATA_SIZE];
    // quaternion of sensor frame relative to auxiliary frame
    for (int i = 0; i < DATA_SIZE; i++) {
        q0[i] = (0.64306622f*FRAC_BASE);
        q1[i] = (0.02828862f*FRAC_BASE);
        q2[i] = (-0.00567953f*FRAC_BASE);
        q3[i] = (-0.76526684f*FRAC_BASE);
    }

    int32_t mag_x[DATA_SIZE], mag_y[DATA_SIZE], mag_z[DATA_SIZE],
            gyr_x[DATA_SIZE], gyr_y[DATA_SIZE], gyr_z[DATA_SIZE],
            acc_x[DATA_SIZE], acc_y[DATA_SIZE], acc_z[DATA_SIZE];
#elif defined(FP_DATA_TYPE)
    float q0[DATA_SIZE], q1[DATA_SIZE], q2[DATA_SIZE], q3[DATA_SIZE];
    // quaternion of sensor frame relative to auxiliary frame
    for (int i = 0; i < DATA_SIZE; i++) {
        q0[i] = 0.64306622f;
        q1[i] = 0.02828862f;
        q2[i] = -0.00567953f;
        q3[i] = -0.76526684f;
    }

    float mag_x[DATA_SIZE], mag_y[DATA_SIZE], mag_z[DATA_SIZE],
          gyr_x[DATA_SIZE], gyr_y[DATA_SIZE], gyr_z[DATA_SIZE],
          acc_x[DATA_SIZE], acc_y[DATA_SIZE], acc_z[DATA_SIZE];
#else
#error "Must set data type: FP or INT"
#endif

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
#if defined(INT_DATA_TYPE)
                    mag_x[row-2] = round(atof(value)*FRAC_BASE);
#elif defined(FP_DATA_TYPE)
                    mag_x[row-2] = atof(value);
#endif
                    break;
                case 2:
#if defined(INT_DATA_TYPE)
                    mag_y[row-2] = round(atof(value)*FRAC_BASE);
#elif defined(FP_DATA_TYPE)
                    mag_y[row-2] = atof(value);
#endif
                    break;
                case 3:
#if defined(INT_DATA_TYPE)
                    mag_z[row-2] = round(atof(value)*FRAC_BASE);
#elif defined(FP_DATA_TYPE)
                    mag_z[row-2] = atof(value);
#endif
                    break;
                case 4:
#if defined(INT_DATA_TYPE)
                    gyr_x[row-2] = round(atof(value)*FRAC_BASE)*61;
#elif defined(FP_DATA_TYPE)
                    gyr_x[row-2] = atof(value)*61;
#endif
                    break;
                case 5:
#if defined(INT_DATA_TYPE)
                    gyr_y[row-2] = round(atof(value)*FRAC_BASE)*61;
#elif defined(FP_DATA_TYPE)
                    gyr_y[row-2] = atof(value)*61;
#endif
                    break;
                case 6:
#if defined(INT_DATA_TYPE)
                    gyr_z[row-2] = round(atof(value)*FRAC_BASE)*61;
#elif defined(FP_DATA_TYPE)
                    gyr_z[row-2] = atof(value)*61;
#endif
                    break;
                case 7:
#if defined(INT_DATA_TYPE)
                    acc_x[row-2] = round(atof(value)*FRAC_BASE)*2;
#elif defined(FP_DATA_TYPE)
                    acc_x[row-2] = atof(value)*2;
#endif
                    break;
                case 8:
#if defined(INT_DATA_TYPE)
                    acc_y[row-2] = round(atof(value)*FRAC_BASE)*2;
#elif defined(FP_DATA_TYPE)
                    acc_y[row-2] = atof(value)*2;
#endif
                    break;
                case 9:
#if defined(INT_DATA_TYPE)
                    acc_z[row-2] = round(atof(value)*FRAC_BASE)*2;
#elif defined(FP_DATA_TYPE)
                    acc_z[row-2] = atof(value)*2;
#endif
                    break;
                default:
                    break;
            }

            value = strtok(NULL, ", ");
            column++;
        }
    }

    fclose(fp);

    u_int64_t time_slots[ITERATION];

    for (size_t idx = 0; idx < ITERATION; idx++) {
        timespec timer = tic();
        for (size_t ts = 0; ts < DATA_SIZE; ts++) {
            MadgwickAHRSupdate(gyr_x[ts], gyr_y[ts], gyr_z[ts],
                               acc_x[ts], acc_y[ts], acc_z[ts],
                               mag_x[ts], mag_y[ts], mag_z[ts],
                               &q0[ts], &q1[ts], &q2[ts], &q3[ts]);
        }
        time_slots[idx] = toc(&timer, "computation delay").tv_nsec;
    }

    u_int64_t average_time = 0;
    for (size_t idx = 0; idx < ITERATION; idx++) {
        average_time += time_slots[idx];
    }
    average_time /= ITERATION;
    printf("average time = %lu nm\n", average_time);

#if defined(FP_DATA_TYPE)
    FILE *fptr = fopen("fp_result.txt", "w");
    for (size_t ts = 0; ts < DATA_SIZE; ts++) {
//        printf("Original: q0[%d]=%f, q1[%d]=%f, q2[%d]=%f, q3[%d]=%f\n",
//               ts, q0[ts], ts, q1[ts], ts, q2[ts], ts, q3[ts]);
        fprintf(fptr, "Original: q0[%d]=%f, q1[%d]=%f, q2[%d]=%f, q3[%d]=%f\n",
               ts, q0[ts], ts, q1[ts], ts, q2[ts], ts, q3[ts]);
    }
    fclose(fptr);
#elif defined(INT_DATA_TYPE)
    FILE *fptr = fopen("int_result.txt", "w");
    for (size_t ts = 0; ts < DATA_SIZE; ts++) {
//        printf("FIX: q0[%d]=%f, q1[%d]=%f, q2[%d]=%f, q3[%d]=%f\n",
//               ts, (double)q0[ts]/FRAC_BASE,
//               ts, (double)q1[ts]/FRAC_BASE,
//               ts, (double)q2[ts]/FRAC_BASE,
//               ts, (double)q3[ts]/FRAC_BASE);
        fprintf(fptr, "FIX: q0[%d]=%f, q1[%d]=%f, q2[%d]=%f, q3[%d]=%f\n",
               ts, (double)q0[ts]/FRAC_BASE,
               ts, (double)q1[ts]/FRAC_BASE,
               ts, (double)q2[ts]/FRAC_BASE,
               ts, (double)q3[ts]/FRAC_BASE);
    }
    fclose(fptr);
//    printf("FIX: q0 = %d.%04d, q1 = %d.%04d, q2 = %d.%04d, q3 = %d.%04d\n",
//           DISPLAY_INT(q0), DISPLAY_FRAC(q0),
//           DISPLAY_INT(q1), DISPLAY_FRAC(q1),
//           DISPLAY_INT(q2), DISPLAY_FRAC(q2),
//           DISPLAY_INT(q3), DISPLAY_FRAC(q3));
#endif
    return 0;
}
