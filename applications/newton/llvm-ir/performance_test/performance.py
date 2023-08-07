# Authored 2022. Pei Mu.
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# *	Redistributions of source code must retain the above
# copyright notice, this list of conditions and the following
# disclaimer.
#
# *	Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following
# disclaimer in the documentation and/or other materials
# provided with the distribution.
#
# *	Neither the name of the author nor the names of its
# contributors may be used to endorse or promote products
# derived from this software without specific prior written
# permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
import platform

import matplotlib.pyplot as plt
import os
import shutil
import seaborn as sns
import palettable
import numpy as np
from matplotlib.ticker import FuncFormatter
import platform

# [instruction counts (million)", "time consumption (s)", "IR lines", "size of library (bytes)"]
y_units = [1000, 1000, 1, 1]

params_num = 10
test_case_num = 10
merit_num = 4
range_extend_num = 6

# changed with auto_test.cpp
range_extend_list = [1, 10, 100, 1000, 10000, 100000]

average_data = []
with open('average_speedup.log', 'r') as f:
    for line in f.readlines():
        line_list = line.strip('\n').split('\t')
        average_data.append(line_list)

average_name_list = []
for i in range(1, len(average_data), range_extend_num):
    name = average_data[i][0][5:]
    average_name_list.append(name)

average_time_speedup = []
average_libsize_reduce = []
for i in range(1, len(average_data), range_extend_num):
    time_speedup = 0
    libsize_reduce = 0
    for j in range(i, i+range_extend_num):
        time_speedup += float(average_data[j][3].strip('%')) / 100
        libsize_reduce += float(average_data[j][5].strip('%')) / 100
    average_time_speedup.append(time_speedup / range_extend_num * 100)
    average_libsize_reduce.append(libsize_reduce / range_extend_num * 100)

assert(len(average_name_list) == len(average_time_speedup))
for i in range(0, len(average_name_list)):
    print(average_name_list[i], "with time speed up: ", format(average_time_speedup[i], '.2f'),
          "%, lib size reduce: ", format(average_libsize_reduce[i], '.2f'), "%")

performance_data = []
with open('perf.log', 'r') as f:
    for line in f.readlines():
        line_list = line.strip('\n').split('\t')
        performance_data.append(line_list)

# prepare data for histogram
name_list = []
for i in range(1, len(performance_data), 3 * params_num):
    name = performance_data[i][0][5:]
    name_list.append(name)
opt_name_list = []
for i in range(2, len(performance_data), 3):
    opt_name_list.append(performance_data[i][0])

param_list = []
for i in range(1, 3 * params_num * range_extend_num, 3):
    lower, upper = map(float, performance_data[i][1].split( ))
    lower_r1 = round(lower, 1)
    upper_r1 = round(upper, 1)
    param_list.append('[' + str(lower_r1) + ', ' + str(upper_r1) + ']')

inst_count = []
for i in range(1, len(performance_data), 3):
    inst_count.append(float(performance_data[i][2]) / y_units[0])
opt_inst_count = []
for i in range(2, len(performance_data), 3):
    opt_inst_count.append(float(performance_data[i][2]) / y_units[0])

time_consumption = []
for i in range(1, len(performance_data), 3):
    time_consumption.append(float(performance_data[i][3]) / y_units[1])
opt_time_consumption = []
for i in range(2, len(performance_data), 3):
    opt_time_consumption.append(float(performance_data[i][3]) / y_units[1])

ir_lines = []
for i in range(1, len(performance_data), 3):
    ir_lines.append(float(performance_data[i][4]) / y_units[2])
opt_ir_lines = []
for i in range(2, len(performance_data), 3):
    opt_ir_lines.append(float(performance_data[i][4]) / y_units[2])

lib_size = []
for i in range(1, len(performance_data), 3):
    lib_size.append(float(performance_data[i][5]) / y_units[3])
opt_lib_size = []
for i in range(2, len(performance_data), 3):
    opt_lib_size.append(float(performance_data[i][5]) / y_units[3])

ori_perf_data = [inst_count, time_consumption, ir_lines, lib_size]
opt_perf_data = [opt_inst_count, opt_time_consumption, opt_ir_lines, opt_lib_size]

# prepare data for heatmap
inst_speedup = []
for i in range(3, len(performance_data), 3):
    inst_speedup.append(float(performance_data[i][2].strip('%')) / 100)
inst_speedup = np.reshape(inst_speedup, (test_case_num, range_extend_num, params_num))

time_speedup = []
for i in range(3, len(performance_data), 3):
    time_speedup.append(float(performance_data[i][3].strip('%')) / 100)
time_speedup = np.reshape(time_speedup, (test_case_num, range_extend_num, params_num))

ir_reduction = []
for i in range(3, len(performance_data), 3):
    ir_reduction.append(float(performance_data[i][4].strip('%')) / 100)
ir_reduction = np.reshape(ir_reduction, (test_case_num, range_extend_num, params_num))

lib_size_reduction = []
for i in range(3, len(performance_data), 3):
    lib_size_reduction.append(float(performance_data[i][5].strip('%')) / 100)
lib_size_reduction = np.reshape(lib_size_reduction, (test_case_num, range_extend_num, params_num))

perf_data_speedup = [inst_speedup, time_speedup, ir_reduction, lib_size_reduction]

y_labels = ["instruction counts (million)", "time (s)", "IR lines", "size (bytes)"]

machine = platform.machine()
# machine = "aarch64"

fig_path = "fig/"
folder = os.path.exists(fig_path)
if folder:
    shutil.rmtree(fig_path, ignore_errors=True)
os.mkdir(fig_path)

# Heatmap
for merit_id in range(1, merit_num, 2):
    for test_case_id in range(test_case_num):
        plt.clf()
        plt.figure(num=merit_id * test_case_num + test_case_id, dpi=300,
                   constrained_layout=True)
        fmt = lambda x, pos: '{:.0%}'.format(x)
        fig = sns.heatmap(data=perf_data_speedup[merit_id][test_case_id].T,
                          cmap=plt.get_cmap('Greens'),
                          annot=True,
                          fmt=".0%",
                          cbar_kws={'format': FuncFormatter(fmt)},
                          xticklabels=range_extend_list,
                          yticklabels=param_list[0:10])
        fig.set_xlabel('Range Extent')
        fig.set_ylabel('Function Parameters')
        plt.title(machine + "-" + name_list[test_case_id * range_extend_num] + "-" + y_labels[merit_id])
        file_name = fig_path + machine + "-" + name_list[test_case_id * range_extend_num] + "-" + y_labels[
            merit_id] + ".png"
        file_name = file_name.replace(" ", "_")
        plt.savefig(file_name)
        plt.close()

os.system('cp perf.log fig/' + machine + "_perf.log")
os.system('cp average_speedup.log fig/' + machine + "_average_speedup.log")
os.system('tar -zvcf fig.tar.gz fig/')
