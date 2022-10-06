#include "Fusion.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include "../CHStone_test/soft_float_api/include/softfloat.h"

#define SAMPLE_PERIOD (0.01f) // replace this with actual sample period

int main() {
    FusionAhrs ahrs;
    FusionAhrsInitialise(&ahrs);

//    while (true) { // this loop should repeat each time new gyroscope data is available
    const FusionVector gyroscope = {0.0f, 0.0f, 0.0f}; // replace this with actual gyroscope data in degrees/s
    const FusionVector accelerometer = {0.0f, 0.0f, 1.0f}; // replace this with actual accelerometer data in g
    for (size_t i = 0; i < 1000000; i++) {
        FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, SAMPLE_PERIOD);

        const FusionEuler euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));
    }


//        printf("Roll %0.1f, Pitch %0.1f, Yaw %0.1f\n", euler.angle.roll, euler.angle.pitch, euler.angle.yaw);
//    }
}
