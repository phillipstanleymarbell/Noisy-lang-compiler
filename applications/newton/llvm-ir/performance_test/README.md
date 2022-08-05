## How to get the performance data automatically

Currently, we use LLVM tools and perf tool to get the performance data, and run it by Makefile.

Here's the command:

```bash
# firstly, change the config of per (tested on Ubuntu 22.04)
sudo vi /etc/sysctl.conf
# add these two lines at the bottom of this file
kernel.kptr_restrict=0
kernel.perf_event_paranoid= -1
# reload config file
sysctl -p /etc/sysctl.conf

# run specific command related the file you want to test
# sudo make "your file without opt"
# sudo make "your file with opt"
# for now, we supported five testcase pair from newlib
make perf_exp
make perf_exp_opt
make perf_log
make perf_log_opt
make perf_acosh
make perf_acosh_opt
make perf_j0
make perf_j0_opt
make perf_y0
make perf_y0_opt

# record your performance data and plot the performance figures
# sorry we need to record the data manually currently
vi result.log
python3 performance.py
```

