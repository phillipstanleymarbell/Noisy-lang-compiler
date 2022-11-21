#!/bin/bash
set -e

#cset shield -c 1,2 -k on
echo 0 > /proc/sys/kernel/randomize_va_space
echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo

test_cases=(perf_acosh_opt perf_exp_opt perf_float64_add_opt perf_float64_div_opt perf_float64_mul_opt perf_float64_sin_opt perf_j0_opt perf_log_opt perf_rem_pio2_opt perf_sincosf_opt perf_y0_opt)
for t in "${test_cases[@]}"
do
	make "$t"
done
