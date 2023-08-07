import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter
from matplotlib.ticker import PercentFormatter


def bar_plot_groups(labels,
                    y1,
                    y2,
                    y1_label,
                    y2_label,
                    ylabel,
                    title,
                    save_fig='',
                    percentage=False):
    """
    A simple wrapper for matplotlib's barplot.

    :param labels:
    :param y1:
    :param y2:
    :param ylabel:
    :param xlabel:
    :param save_fig:
    :return:
    """
    xlabel = labels + labels
    x = np.arange(len(xlabel))  # the label locations
    width = 0.35  # the width of the bars

    plt.rc('text', usetex=True)
    plt.rc('font', family='serif')
    plt.rc('font', size=12)

    fig, ax = plt.subplots()
    # ax.bar(x - width / 2, y1[0:len(x)], width, label=y1_label, color='#c2a4cf')
    # ax.bar(x + width / 2, y2[0:len(x)], width, label=y2_label, color='#7b3294')
    ax.bar(x[0:len(labels)] - width / 2, y1[0:len(labels)], width, label="arm "+y1_label, color='#fdb863')
    ax.bar(x[0:len(labels)] + width / 2, y2[0:len(labels)], width, label="arm "+y2_label, color='#e66101')
    ax.bar(x[len(labels):] - width / 2, y1[len(labels):], width, label="x86 "+y1_label, color='#c2a4cf')
    ax.bar(x[len(labels):] + width / 2, y2[len(labels):], width, label="x86 "+y2_label, color='#7b3294')

    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel(ylabel)
    if percentage:
        ax.yaxis.set_major_formatter(PercentFormatter(1))
        ax.set_ylim([-0.05, max(max(y1), max(y2))*1.1])
    else:
        ax.set_ylim([min(min(y1), min(y2))*0.8, max(max(y1), max(y2))*1.1])
    ax.set_title(title)
    ax.set_xticks(x)
    ax.set_xticklabels(xlabel)
    ax.legend(prop={'size': 9}, loc='upper right')

    plt.xticks(rotation=45)
    ax.yaxis.grid()  # horizontal lines

    fig.tight_layout()

    if len(save_fig):
        plt.savefig(save_fig)
    else:
        plt.show()


if __name__ == '__main__':
    labels = ["rem_pio2", "sincosf", "float64_add", "float64_div", "float64_mul"]

    # time speedup
    arm_without_bitwise = [1.04, 1, 1.25, 1.16, 1.22]
    arm_with_bitwise = [1.11, 1.39, 2.01, 1.41, 1.33]
    x86_without_bitwise = [1.07, 1.05, 1.18, 1.06, 1.12]
    x86_with_bitwise = [1.13, 1.48, 1.47, 1.14, 1.20]
    y1 = arm_without_bitwise + x86_without_bitwise
    y2 = arm_with_bitwise + x86_with_bitwise

    bar_plot_groups(labels, y1, y2, 'w/o Bitwise Op Analysis',
                    'w/ Bitwise Op Analysis', 'Speedup',
                    'Speedup with/without bitwise operator analysis',
                    'bitwise_op_time_comparison.png')

    # size reduction
    arm_without_bitwise = [1-0.71, 1-1, 1-1, 1-0.86, 1-0.89]
    arm_with_bitwise = [1-0.8, 1-0.92, 1-0.98, 1-0.79, 1-0.98]
    x86_without_bitwise = [1-0.46, 1-1, 1-1, 1-0.86, 1-0.86]
    x86_with_bitwise = [1-0.65, 1-0.91, 1-0.99, 1-0.79, 1-0.94]
    y1 = arm_without_bitwise + x86_without_bitwise
    y2 = arm_with_bitwise + x86_with_bitwise

    bar_plot_groups(labels, y1, y2, 'w/o Bitwise Op Analysis',
                    'w/ Bitwise Op Analysis', 'Reduction',
                    'Size reduction with/without bitwise operator analysis',
                    'bitwise_op_size_comparison.png', percentage=True)
