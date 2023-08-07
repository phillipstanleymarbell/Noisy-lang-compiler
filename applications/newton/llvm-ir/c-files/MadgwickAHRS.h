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

#include <math.h>
#include <stdio.h>

//----------------------------------------------------------------------------------------------------
// Variable declaration

// extern volatile float beta;				// algorithm gain
// extern volatile float q0, q1, q2, q3;	// quaternion of sensor frame relative to auxiliary frame

//---------------------------------------------------------------------------------------------------
// Function declarations

void MadgwickAHRSupdate(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz,
                        float* q0_ptr, float* q1_ptr, float* q2_ptr, float* q3_ptr);
void MadgwickAHRSupdateIMU(float gx, float gy, float gz, float ax, float ay, float az,
                           float* q0_ptr, float* q1_ptr, float* q2_ptr, float* q3_ptr);

#endif
//=====================================================================================================
// End of file
//=====================================================================================================
