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

import matplotlib.pyplot as plt

performance_data = []
with open('result.log', 'r') as f:
    for line in f.readlines():
        line_list = line.strip('\n').split('\t')
        performance_data.append(line_list)

name_list = []
for i in range(1, len(performance_data), 2):
    name_list.append(performance_data[i][0])
opt_name_list = []
for i in range(2, len(performance_data), 2):
    opt_name_list.append(performance_data[i][0])

fig = plt.figure(1)

y_labels = ["instruction counts (million)", "IR lines", "time elapsed (s)", "bytes"]
y_ranges = [(130, 600), (100, 1600), (0.01, 0.10), (3000, 16000)]
y_units = [1000000, 1, 1, 1]

for fig_id in range(4):
    ax1 = plt.subplot(2, 2, fig_id + 1)

    count_list = []
    for i in range(1, len(performance_data), 2):
        count_list.append(float(performance_data[i][fig_id + 1]) / y_units[fig_id])
    opt_count_list = []
    for i in range(2, len(performance_data), 2):
        opt_count_list.append(float(performance_data[i][fig_id + 1]) / y_units[fig_id])
    x = list(range(len(count_list)))
    total_width, n = 0.5, 2
    width = total_width / n

    plt.ylim(y_ranges[fig_id])
    plt.ylabel(y_labels[fig_id])
    plt.bar(x, count_list, width=width, label='basic performance', tick_label=name_list, fc='y')
    for i in range(len(x)):
        x[i] = x[i] + width
    plt.bar(x, opt_count_list, width=width, label='optimized performance', fc='r')
    plt.legend()
plt.show()
