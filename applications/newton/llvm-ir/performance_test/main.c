//
// Created by ds123 on 2022/6/10.
//

#include <stdio.h>
#include <stdlib.h>

// random floating point, [min, max]
float randomFloat(float min, float max) {
	float randFpValue = min + 1.0 * rand() / RAND_MAX * (max - min);
	return randFpValue;
}

int main() {
    double result = 0;
    /*
     * I try to pass the function name from command line to make it more automatic,
     * but it's seemingly forbidden in C/C++.
     * So we need to write the function name manually here.
     * */
    for (int i = 0; i < 1000000; i++) {
//        result = controlFlowFunc(randomFloat(-16.0, 16.0));
//        result = libc_exp(randomFloat(2.0, 10.0));
//        result = libc_log(randomFloat(2.0, 10.0));
//        result = libc_acosh(randomFloat(2.0, 10.0));
//        result = libc_j0(randomFloat(2.0, 10.0));
        result = libc_y0(randomFloat(2.0, 10.0));
    }

	return 0;
}
