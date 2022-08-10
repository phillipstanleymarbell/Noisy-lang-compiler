cset shield -c 1,2 -k on
echo 0 > /proc/sys/kernel/randomize_va_space
echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo
