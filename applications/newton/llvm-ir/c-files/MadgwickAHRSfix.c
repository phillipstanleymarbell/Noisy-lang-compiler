#include "MadgwickAHRSfix.h"

#define sampleFreq	28		// sample frequency in Hz
#define betaDef		0.1f		// 2 * proportional gain

typedef int32_t bmx055xAcceleration;
typedef int32_t bmx055yAcceleration;
typedef int32_t bmx055zAcceleration;
typedef int32_t bmx055xAngularRate;
typedef int32_t bmx055yAngularRate;
typedef int32_t bmx055zAngularRate;
typedef int32_t bmx055xMagneto;
typedef int32_t bmx055yMagneto;
typedef int32_t bmx055zMagneto;

volatile bmx055xAcceleration	beta = (uint8_t)(betaDef*FRAC_BASE);	//0.1f				// 2 * proportional gain (Kp)
//volatile bmx055xAcceleration	q0 = 0.64306622f*FRAC_BASE, q1 = 0.02828862f*FRAC_BASE,
//                                q2 = -0.00567953f*FRAC_BASE, q3 = -0.76526684f*FRAC_BASE;	// quaternion of sensor frame relative to auxiliary frame

// m=-7 1/Yest=0.0858 Yest=11.3120 Yest_hex=B50
// m=-6 1/Yest=0.1248 Yest=8.0000 Yest_hex=800
// m=-5 1/Yest=0.1755 Yest=5.6552 Yest_hex=5A8
// m=-4 1/Yest=0.2496 Yest=4.0000 Yest_hex=400
// m=-3 1/Yest=0.3510 Yest=2.8268 Yest_hex=2D4
// m=-2 1/Yest=0.4992 Yest=2.0000 Yest_hex=200
// m=-1 1/Yest=0.7059 Yest=1.4134 Yest_hex=16A
// m=0 1/Yest=1.0000 Yest=1.0000 Yest_hex=100
// m=1 1/Yest=1.4134 Yest=0.7059 Yest_hex=B5
// m=2 1/Yest=2.0000 Yest=0.4992 Yest_hex=80
// m=3 1/Yest=2.8268 Yest=0.3510 Yest_hex=5A
// m=4 1/Yest=4.0000 Yest=0.2496 Yest_hex=40
// m=5 1/Yest=5.6552 Yest=0.1755 Yest_hex=2D
// m=6 1/Yest=8.0000 Yest=0.1248 Yest_hex=20
// m=7 1/Yest=11.3120 Yest=0.0858 Yest_hex=16
// m=8 1/Yest=16.0000 Yest=0.0624 Yest_hex=10
// m=9 1/Yest=22.6240 Yest=0.0429 Yest_hex=B
// m=10 1/Yest=32.0000 Yest=0.0312 Yest_hex=8
// m=11 1/Yest=45.2496 Yest=0.0195 Yest_hex=5
// m=12 1/Yest=64.0000 Yest=0.0156 Yest_hex=4
// m=13 1/Yest=90.4992 Yest=0.0078 Yest_hex=2
// m=14 1/Yest=128.0000 Yest=0.0078 Yest_hex=2
// m=15 1/Yest=181.0000 Yest=0.0039 Yest_hex=1
// m=16 1/Yest=256.0000 Yest=0.0039 Yest_hex=1
// m=17 1/Yest=362.0000 Yest=0.0000 Yest_hex=0
// m=18 1/Yest=512.0000 Yest=0.0000 Yest_hex=0
// m=19 1/Yest=724.0000 Yest=0.0000 Yest_hex=0
// m=20 1/Yest=1024.0000 Yest=0.0000 Yest_hex=0
// m=21 1/Yest=1448.0000 Yest=0.0000 Yest_hex=0
// m=22 1/Yest=2048.0000 Yest=0.0000 Yest_hex=0

int32_t
mulfix(int32_t x, int32_t y)
{
//    int32_t result;
//    int64_t temp;
//    temp = (int64_t)x * (int64_t)y;
//    temp += K;
//    result = round(temp/FRAC_BASE);
//    return result;
//    return ((int64_t)(x*y)) > 0 ? ((int64_t)(x*y))>>FRAC_Q : (((int64_t)(x*y))>>FRAC_Q)+1;
    return ((int64_t)x*y)>>FRAC_Q;
}

/*
 *	Compute square root of x and reciprocal with Goldschmidt's method
 */
int32_t
sqrt_rsqrt(int32_t x, int recip) {
    if (recip) {
        int32_t int_halfx = mulfix(0.5*FRAC_BASE, x);
        float fp_y = (float)x/FRAC_BASE;
        long i = *(long*)&fp_y;
        i = 0x5f3759df - (i>>1);
        fp_y = *(float*)&i;
        int32_t int_y = fp_y*FRAC_BASE;
        int_y = mulfix(int_y, ((int32_t)(1.5f*FRAC_BASE) - (mulfix(mulfix(int_halfx, int_y), int_y))));
        return int_y;
//        fp_y = fp_y * (1.5f - (halfx * fp_y * fp_y));
//        return fp_y*FRAC_BASE;
    } else {
        int32_t res = (int32_t)sqrt((double)x)<<(FRAC_Q/2);
        if (FRAC_Q%2)
            return res*1.414213562;
        else
            return res;
    }
}

//====================================================================================================
// Functions

//---------------------------------------------------------------------------------------------------
// AHRS algorithm update

void 
MadgwickAHRSupdate(bmx055xAngularRate gx, bmx055yAngularRate gy, bmx055zAngularRate gz,
                   bmx055xAcceleration ax, bmx055yAcceleration ay, bmx055zAcceleration az,
                   bmx055xMagneto mx, bmx055yMagneto my, bmx055zMagneto mz,
                   int32_t* q0_ptr, int32_t* q1_ptr, int32_t* q2_ptr, int32_t* q3_ptr) {

    int32_t q0 = *q0_ptr;
    int32_t q1 = *q1_ptr;
    int32_t q2 = *q2_ptr;
    int32_t q3 = *q3_ptr;

	int32_t		recipNorm;
	int32_t		s0, s1, s2, s3;
	int32_t		qDot1, qDot2, qDot3, qDot4;
	int32_t		hx, hy;
	int32_t		_2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _2q0, _2q1, _2q2, _2q3, _2q0q2, _2q2q3, q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3, q2q2, q2q3, q3q3;

	// Use IMU algorithm if magnetometer measurement invalid (avoids NaN in magnetometer normalisation)
	if((mx == 0x0) && (my == 0x0) && (mz == 0x0)) {
		MadgwickAHRSupdateIMU(gx, gy, gz, ax, ay, az, q0_ptr, q1_ptr, q2_ptr, q3_ptr);
		return;
	}

	// Rate of change of quaternion from gyroscope
	qDot1 = (-mulfix(q1, gx) - mulfix(q2, gy) - mulfix(q3, gz)) / 2;
	qDot2 = (mulfix(q0, gx) + mulfix(q2, gz) - mulfix(q3, gy)) / 2;
	qDot3 = (mulfix(q0, gy) - mulfix(q1, gz) + mulfix(q3, gx)) / 2;
	qDot4 = (mulfix(q0, gz) + mulfix(q1, gy) - mulfix(q2, gx)) / 2;

	// Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
	if(!((ax == 0x0) && (ay == 0x0) && (az == 0x0))) {

		// Normalise accelerometer measurement
		recipNorm = sqrt_rsqrt(mulfix(ax, ax) + mulfix(ay, ay) + mulfix(az, az), true);
//        printf("1: %f\n", (double)recipNorm/FRAC_BASE);
		ax = mulfix(ax, recipNorm);
		ay = mulfix(ay, recipNorm);
		az = mulfix(az, recipNorm);

		// Normalise magnetometer measurement
		recipNorm = sqrt_rsqrt(mulfix(mx, mx) + mulfix(my, my) + mulfix(mz, mz), true);
		mx = mulfix(mx, recipNorm);
		my = mulfix(my, recipNorm);
		mz = mulfix(mz, recipNorm);

		// Auxiliary variables to avoid repeated arithmetic
		_2q0mx = 2 * mulfix(q0, mx);
		_2q0my = 2 * mulfix(q0, my);
		_2q0mz = 2 * mulfix(q0, mz);
		_2q1mx = 2 * mulfix(q1, mx);
		_2q0 = 2*q0;
		_2q1 = 2*q1;
		_2q2 = 2*q2;
		_2q3 = 2*q3;
		_2q0q2 = 2 * mulfix(q0, q2);
		_2q2q3 = 2 * mulfix(q2, q3);
		q0q0 = mulfix(q0, q0);
		q0q1 = mulfix(q0, q1);
		q0q2 = mulfix(q0, q2);
		q0q3 = mulfix(q0, q3);
		q1q1 = mulfix(q1, q1);
		q1q2 = mulfix(q1, q2);
		q1q3 = mulfix(q1, q3);
		q2q2 = mulfix(q2, q2);
		q2q3 = mulfix(q2, q3);
		q3q3 = mulfix(q3, q3);

		// Reference direction of Earth's magnetic field
		hx = mulfix(mx, q0q0)
			- mulfix(_2q0my, q3)
			+ mulfix(_2q0mz, q2)
			+ mulfix(mx, q1q1)
			+ mulfix(mulfix(_2q1, my), q2)
			+ mulfix(mulfix(_2q1, mz), q3)
			- mulfix(mx, q2q2) 
			- mulfix(mx, q3q3);
		hy = mulfix(_2q0mx, q3)
			+ mulfix(my, q0q0)
			- mulfix(_2q0mz, q1)
			+ mulfix(_2q1mx, q2)
			- mulfix(my, q1q1)
			+ mulfix(my, q2q2)
			+ mulfix(mulfix(_2q2, mz), q3)
			- mulfix(my, q3q3);
		_2bx = sqrt_rsqrt(mulfix(hx, hx) + mulfix(hy, hy), false);
		_2bz = -mulfix(_2q0mx, q2)
			+ mulfix(_2q0my, q1)
			+ mulfix(mz, q0q0)
			+ mulfix(_2q1mx, q3)
			- mulfix(mz, q1q1)
			+ mulfix(mulfix(_2q2, my), q3)
			- mulfix(mz, q2q2)
			+ mulfix(mz, q3q3);
		_4bx = 2*_2bx;
		_4bz = 2*_2bz;

		// Gradient decent algorithm corrective step
		s0 = 	- mulfix(_2q2, (2*q1q3 - _2q0q2 - ax))
			+ mulfix(_2q1, (2*q0q1 + _2q2q3 - ay))
			- mulfix(mulfix(_2bz, q2), (
					  mulfix(_2bx, (DEC2FRAC(0.5) - q2q2 - q3q3))
					+ mulfix(_2bz, (q1q3 - q0q2))
					- mx))
			+ mulfix((-mulfix(_2bx, q3) + mulfix(_2bz, q1)), (
				  mulfix(_2bx, (q1q2 - q0q3))
				+ mulfix(_2bz, (q0q1 + q2q3)) 
				- my))
			+ mulfix(mulfix(_2bx, q2), (
				  mulfix(_2bx, (q0q2 + q1q3))
				+ mulfix(_2bz, (DEC2FRAC(0.5) - q1q1 - q2q2))
				- mz));
		s1 = 	  mulfix(_2q3, (2*q1q3 - _2q0q2 - ax))
			+ mulfix(_2q0, (2*q0q1 + _2q2q3 - ay))
			- 4 * mulfix(q1, (FRAC_BASE - 2*q1q1 - 2*q2q2 - az))
			+ mulfix(mulfix(_2bz, q3), (
				  mulfix(_2bx, (DEC2FRAC(0.5) - q2q2 - q3q3))
				+ mulfix(_2bz, (q1q3 - q0q2))
				- mx))
			+ mulfix(mulfix(_2bx, q2) + mulfix(_2bz, q0), (
				  mulfix(_2bx, (q1q2 - q0q3))
				+ mulfix(_2bz, (q0q1 + q2q3))
				- my))
			+ mulfix(mulfix(_2bx, q3) - mulfix(_4bz, q1), (
				  mulfix(_2bx, (q0q2 + q1q3))
				+ mulfix(_2bz, (DEC2FRAC(0.5) - q1q1 - q2q2))
				- mz));
		s2 =    - mulfix(_2q0, (2*q1q3 - _2q0q2 - ax))
			+ mulfix(_2q3, (2*q0q1 + _2q2q3 - ay))
			- 4 * mulfix(q2, (FRAC_BASE - 2*q1q1 - 2*q2q2 - az))
			+ mulfix((-mulfix(_4bx, q2) - mulfix(_2bz, q0)), (
				  mulfix(_2bx, (DEC2FRAC(0.5) - q2q2 - q3q3))
				+ mulfix(_2bz, (q1q3 - q0q2))
				- mx))
			+ mulfix((mulfix(_2bx, q1) + mulfix(_2bz, q3)), (
				  mulfix(_2bx, (q1q2 - q0q3))
				+ mulfix(_2bz, (q0q1 + q2q3))
				- my))
			+ mulfix((mulfix(_2bx, q0) - mulfix(_4bz, q2)), (
				mulfix(_2bx, (q0q2 + q1q3))
				+ mulfix(_2bz, (DEC2FRAC(0.5) - q1q1 - q2q2))
				- mz));
		s3 =      mulfix(_2q1, (2*q1q3 - _2q0q2 - ax))
			+ mulfix(_2q2, (2*q0q1 + _2q2q3 - ay))
			+ mulfix((-mulfix(_4bx, q3) + mulfix(_2bz, q1)), (
				  mulfix(_2bx, (DEC2FRAC(0.5) - q2q2 - q3q3))
				+ mulfix(_2bz, (q1q3 - q0q2))
				- mx))
			+ mulfix((-mulfix(_2bx, q0) + mulfix(_2bz, q2)), (
				  mulfix(_2bx, (q1q2 - q0q3))
				+ mulfix(_2bz, (q0q1 + q2q3))
				- my))
			+ mulfix(mulfix(_2bx, q1), (
				mulfix(_2bx, (q0q2 + q1q3))
				 + mulfix(_2bz, (DEC2FRAC(0.5) - q1q1 - q2q2))
				 - mz));
		recipNorm = sqrt_rsqrt(mulfix(s0, s0) + mulfix(s1, s1) + mulfix(s2, s2) + mulfix(s3, s3), true); // normalise step magnitude
		s0 = mulfix(s0, recipNorm);
		s1 = mulfix(s1, recipNorm);
		s2 = mulfix(s2, recipNorm);
		s3 = mulfix(s3, recipNorm);
		
		/* 2nd iter normalizaton */
		// recipNorm = sqrt_rsqrt(mulfix(s0, s0) + mulfix(s1, s1) + mulfix(s2, s2) + mulfix(s3, s3), true); // normalise step magnitude
		// s0 = mulfix(s0, recipNorm);
		// s1 = mulfix(s1, recipNorm);
		// s2 = mulfix(s2, recipNorm);
		// s3 = mulfix(s3, recipNorm);

		// Apply feedback step
		qDot1 -= mulfix(beta, s0);
		qDot2 -= mulfix(beta, s1);
		qDot3 -= mulfix(beta, s2);
		qDot4 -= mulfix(beta, s3);
	}

	// Integrate rate of change of quaternion to yield quaternion
	q0 += qDot1 / sampleFreq;
	q1 += qDot2 / sampleFreq;
	q2 += qDot3 / sampleFreq;
	q3 += qDot4 / sampleFreq;

	// Normalise quaternion
	recipNorm = sqrt_rsqrt(mulfix(q0, q0) + mulfix(q1, q1) + mulfix(q2, q2) + mulfix(q3, q3), true);
//    printf("q0=%f, q1=%f, q2=%f, q3=%f, recipNorm=%f\n",
//           (double)q0/FRAC_BASE,
//           (double)q1/FRAC_BASE,
//           (double)q2/FRAC_BASE,
//           (double)q3/FRAC_BASE,
//           (double)recipNorm/FRAC_BASE);
	q0 = mulfix(q0, recipNorm);
	q1 = mulfix(q1, recipNorm);
	q2 = mulfix(q2, recipNorm);
	q3 = mulfix(q3, recipNorm);
    *q0_ptr = q0;
    *q1_ptr = q1;
    *q2_ptr = q2;
    *q3_ptr = q3;

//    printf("FIX: q0 = %d.%04d, q1 = %d.%04d, q2 = %d.%04d, q3 = %d.%04d\n",
//           DISPLAY_INT(q0), DISPLAY_FRAC(q0),
//           DISPLAY_INT(q1), DISPLAY_FRAC(q1),
//           DISPLAY_INT(q2), DISPLAY_FRAC(q2),
//           DISPLAY_INT(q3), DISPLAY_FRAC(q3));
	
	// /* 2nd iter normalization */
	// recipNorm = sqrt_rsqrt(mulfix(q0, q0) + mulfix(q1, q1) + mulfix(q2, q2) + mulfix(q3, q3), true);
	// q0 = mulfix(q0, recipNorm);
	// q1 = mulfix(q1, recipNorm);
	// q2 = mulfix(q2, recipNorm);
	// q3 = mulfix(q3, recipNorm);
}

//---------------------------------------------------------------------------------------------------
// IMU algorithm update

void 
MadgwickAHRSupdateIMU(bmx055xAngularRate gx, bmx055yAngularRate gy, bmx055zAngularRate gz,
                      bmx055xAcceleration ax, bmx055yAcceleration ay, bmx055zAcceleration az,
                      int32_t* q0_ptr, int32_t* q1_ptr, int32_t* q2_ptr, int32_t* q3_ptr) {
    int32_t q0 = *q0_ptr;
    int32_t q1 = *q1_ptr;
    int32_t q2 = *q2_ptr;
    int32_t q3 = *q3_ptr;
	int32_t		recipNorm;
	int32_t		s0, s1, s2, s3;
	int32_t		qDot1, qDot2, qDot3, qDot4;
	int32_t		_2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2 ,_8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

	// Rate of change of quaternion from gyroscope
	qDot1 = (- mulfix(q1, gx) - mulfix(q2, gy) - mulfix(q3, gz)) / 2;
	qDot2 = (  mulfix(q0, gx) + mulfix(q2, gz) - mulfix(q3, gy)) / 2;
	qDot3 = (  mulfix(q0, gy) - mulfix(q1, gz) + mulfix(q3, gx)) / 2;
	qDot4 = (  mulfix(q0, gz) + mulfix(q1, gy) - mulfix(q2, gx)) / 2;

	// Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
	if(!((ax == 0x0) && (ay == 0x0) && (az == 0x0))) {

		// Normalise accelerometer measurement
		recipNorm = sqrt_rsqrt(mulfix(ax, ax) + mulfix(ay, ay) + mulfix(az, az), true);
		ax = mulfix(ax, recipNorm);
		ay = mulfix(ay, recipNorm);
		az = mulfix(az, recipNorm);

		// Auxiliary variables to avoid repeated arithmetic
		_2q0 = 2*q0;
		_2q1 = 2*q1;
		_2q2 = 2*q2;
		_2q3 = 2*q3;
		_4q0 = 4*q0;
		_4q1 = 4*q1;
		_4q2 = 4*q2;
		_8q1 = 8*q1;
		_8q2 = 8*q2;
		q0q0 = mulfix(q0, q0);
		q1q1 = mulfix(q1, q1);
		q2q2 = mulfix(q2, q2);
		q3q3 = mulfix(q3, q3);

		// Gradient decent algorithm corrective step
		s0 = 	  mulfix(_4q0, q2q2) 
			+ mulfix(_2q2, ax)
			+ mulfix(_4q0, q1q1) 
			- mulfix(_2q1, ay);
		s1 = 	  mulfix(_4q1, q3q3) 
			- mulfix(_2q3, ax)
			+ 4 * mulfix(q0q0, q1) 
			- mulfix(_2q0, ay) 
			- _4q1 
			+ mulfix(_8q1, q1q1) 
			+ mulfix(_8q1, q2q2) 
			+ mulfix(_4q1, az);
		s2 = 4 *  mulfix(q0q0, q2)
			+ mulfix(_2q0, ax) 
			+ mulfix(_4q2, q3q3) 
			- mulfix(_2q3, ay) 
			- _4q2 
			+ mulfix(_8q2, q1q1) 
			+ mulfix(_8q2, q2q2) 
			+ mulfix(_4q2, az);
		s3 = 4 *  mulfix(q1q1, q3) 
			- mulfix(_2q1, ax) 
			+ 4 * mulfix(q2q2, q3) 
			- mulfix(_2q2, ay);
		recipNorm = sqrt_rsqrt(mulfix(s0, s0) + mulfix(s1, s1) + mulfix(s2, s2) + mulfix(s3, s3), true); // normalise step magnitude
		s0 = mulfix(s0, recipNorm);
		s1 = mulfix(s1, recipNorm);
		s2 = mulfix(s2, recipNorm);
		s3 = mulfix(s3, recipNorm);

		// Apply feedback step
		qDot1 -= mulfix(beta, s0);
		qDot2 -= mulfix(beta, s1);
		qDot3 -= mulfix(beta, s2);
		qDot4 -= mulfix(beta, s3);
	}

	// Integrate rate of change of quaternion to yield quaternion
	q0 += qDot1 / sampleFreq;
	q1 += qDot2 / sampleFreq;
	q2 += qDot3 / sampleFreq;
	q3 += qDot4 / sampleFreq;

	// Normalise quaternion
	recipNorm = sqrt_rsqrt(mulfix(q0, q0) + mulfix(q1, q1) + mulfix(q2, q2) + mulfix(q3, q3), true);
	q0 = mulfix(q0, recipNorm);
	q1 = mulfix(q1, recipNorm);
	q2 = mulfix(q2, recipNorm);
	q3 = mulfix(q3, recipNorm);
    q0_ptr = &q0;
    q1_ptr = &q1;
    q2_ptr = &q2;
    q3_ptr = &q3;
}
