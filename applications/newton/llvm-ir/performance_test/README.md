## How to get the performance data automatically

Currently, we use LLVM tools and perf tool to get the performance data, and run it by Makefile.

Here's the command:

```bash
# clean the current directory
make clean
# clean and create .ll file
make
# change the API function you want to test in main.c. sorry, this step should be done manually
vi main.c
# get the non_optimized files
make non_optimized AIM_FILE="your file" # eg. make non_optimized AIM_FILE=e_log
# get the non_optimized performance data
sudo make all
# get the optimized files
make optimized AIM_FILE="your file" # eg. make optimized AIM_FILE=e_log
# make all again to get the optimized performance data
sudo make all

# plot the performance figures
python3 performance.py
```

