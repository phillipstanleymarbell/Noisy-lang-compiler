#include "Fusion.h"
#include <stdbool.h>
#include <stdio.h>
#include "../CHStone_test/dfadd/include/milieu.h"
#include "../CHStone_test/dfadd/include/softfloat.h"

#define SAMPLE_PERIOD (0.01f) // replace this with actual sample period

int main() {
    FusionAhrs ahrs;
    FusionAhrsInitialise(&ahrs);

    while (true) { // this loop should repeat each time new gyroscope data is available
        const FusionVector gyroscope = {0.0f, 0.0f, 0.0f}; // replace this with actual gyroscope data in degrees/s
        const FusionVector accelerometer = {0.0f, 0.0f, 1.0f}; // replace this with actual accelerometer data in g

        FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, SAMPLE_PERIOD);

        const FusionEuler euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));
        printf("float_add: %ld\n", float64_add(0x3FF0000000000000ULL, 0x3FF0000000000000ULL));
        printf("float_mul: %ld\n", float64_mul(0x3FF0000000000000ULL, 0x3FF0000000000000ULL));
        printf("float_div: %ld\n", float64_div(0x3FF0000000000000ULL, 0x3FF0000000000000ULL));

//        printf("Roll %0.1f, Pitch %0.1f, Yaw %0.1f\n", euler.angle.roll, euler.angle.pitch, euler.angle.yaw);
    }
}
