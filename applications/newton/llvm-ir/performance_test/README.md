## How to get the performance data automatically

Currently, we use LLVM tools and perf tool to get the performance data, and run it by Makefile.

Here's the command:

```bash
# run specific command related the file you want to test
# sudo make "your file without opt"
# sudo make "your file with opt"
# for now, we supported five testcase pair from newlib
sudo make perf_exp
sudo make perf_exp_opt
sudo make perf_log
sudo make perf_log_opt
sudo make perf_acosh
sudo make perf_acosh_opt
sudo make perf_j0
sudo make perf_j0_opt
sudo make perf_y0
sudo make perf_y0_opt

# collect the line count of IR
wc -l out.ll

# record your performance data and plot the performance figures
# sorry we need to record the data manually currently
vi result.log
python3 performance.py
```

