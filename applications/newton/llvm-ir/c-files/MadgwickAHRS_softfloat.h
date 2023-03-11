//=====================================================================================================
// MadgwickAHRS.h
//=====================================================================================================
//
// Implementation of Madgwick's IMU and AHRS algorithms.
// See: http://www.x-io.co.uk/node/8#open_source_ahrs_and_imu_algorithms
//
// Date			Author          Notes
// 29/09/2011	SOH Madgwick    Initial release
// 02/10/2011	SOH Madgwick	Optimised for reduced CPU load
//
//=====================================================================================================
#ifndef MadgwickAHRS_h
#define MadgwickAHRS_h

#include "../CHStone_test/soft_float_api/include/softfloat.h"

//----------------------------------------------------------------------------------------------------
// Variable declaration

// extern volatile float beta;				// algorithm gain
// extern volatile float q0, q1, q2, q3;	// quaternion of sensor frame relative to auxiliary frame

//---------------------------------------------------------------------------------------------------
// Function declarations

#define FRAC_Q			8
#define FRAC_BASE		(1<<FRAC_Q)
#define DEC2FRAC(_d)		((uint8_t)(_d*FRAC_BASE))

//typedef float bmx055xAcceleration;
//typedef float bmx055yAcceleration;
typedef float bmx055zAcceleration;
typedef float bmx055xAngularRate;
typedef float bmx055yAngularRate;
typedef float bmx055zAngularRate;
typedef float bmx055xMagneto;
typedef float bmx055yMagneto;
typedef float bmx055zMagneto;

void MadgwickAHRSupdate(bmx055xAngularRate gx, bmx055yAngularRate gy, bmx055zAngularRate gz,
                        bmx055xAcceleration ax, bmx055yAcceleration ay, bmx055zAcceleration az,
                        bmx055xMagneto mx, bmx055yMagneto my, bmx055zMagneto mz);
void MadgwickAHRSupdateIMU(bmx055xAngularRate gx, bmx055yAngularRate gy, bmx055zAngularRate gz,
                           bmx055xAcceleration ax, bmx055yAcceleration ay, bmx055zAcceleration az);

#endif
//=====================================================================================================
// End of file
//=====================================================================================================
