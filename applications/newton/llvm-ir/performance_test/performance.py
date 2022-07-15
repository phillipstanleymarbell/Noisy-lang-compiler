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

y_labels = ["instruction counts (million)", "branch counts (million)", "time elapsed (s)", "bytes"]
y_ranges = [(100, 600), (20, 90), (0.01, 0.10), (3000, 16000)]
y_units = [1000000, 1000000, 1, 1]

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
