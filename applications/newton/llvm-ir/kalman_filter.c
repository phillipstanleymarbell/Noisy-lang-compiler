#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DT 0.01    // time step
#define ACC_VAR 0.1 // acceleration measurement variance
#define GYRO_VAR 0.1 // gyro measurement variance
#define Q_ANGLE 0.1  // process noise covariance for the angle
#define Q_BIAS 0.01  // process noise covariance for the gyro bias
#define R_ANGLE 0.1  // measurement noise covariance for the angle

typedef struct {
    double angle;
    double bias;
    double rate;
    double P[2][2];
} kalman_state;

// update the state of the Kalman filter based on a new measurement
void kalman_update(kalman_state *state, double acc, double gyro) {
    double y, S;
    double K[2];

    // update angle and bias estimates using the gyro measurement
    state->angle += DT * (gyro - state->bias);
    state->bias += Q_BIAS * DT;

    // calculate the innovation and innovation covariance
    y = acc - state->angle;
    S = state->P[0][0] + R_ANGLE;

    // calculate the Kalman gain
    K[0] = state->P[0][0] / S;
    K[1] = state->P[1][0] / S;

    // update the state estimate and covariance
    state->angle += K[0] * y;
    state->bias += K[1] * y;
    state->P[0][0] -= K[0] * state->P[0][0];
    state->P[0][1] -= K[0] * state->P[0][1];
    state->P[1][0] -= K[1] * state->P[0][0];
    state->P[1][1] -= K[1] * state->P[0][1];

    // propagate the state estimate and covariance using the gyro measurement
    state->rate = gyro - state->bias;
    state->angle += DT * state->rate;
    state->P[0][0] += DT * (DT*state->P[1][1] - state->P[0][1] - state->P[1][0] + Q_ANGLE);
    state->P[0][1] -= DT * state->P[1][1];
    state->P[1][0] -= DT * state->P[1][1];
    state->P[1][1] += Q_ANGLE * DT;
}

int main() {
    double acc, gyro;
    kalman_state state = {0, 0, 0, {{0, 0}, {0, 0}}};

    // simulate some sensor measurements
    for (int i = 0; i < 1000; i++) {
        acc = 1.0 + ACC_VAR * (rand() / (double)RAND_MAX - 0.5);
        gyro = 0.1 + GYRO_VAR * (rand() / (double)RAND_MAX - 0.5);
        kalman_update(&state, acc, gyro);
        printf("%f %f\n", state.angle, state.rate);
    }

    return 0;
}