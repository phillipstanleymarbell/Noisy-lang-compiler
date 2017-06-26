from math import sqrt

# https://stackoverflow.com/questions/15389768/standard-deviation-of-a-list
def mean(data):
    """Return the sample arithmetic mean of data."""
    n = len(data)
    if n < 1:
        raise ValueError('mean requires at least one data point')
    return sum(data)/n # in Python 2 use sum(data)/float(n)

# http://codeselfstudy.com/blogs/how-to-calculate-standard-deviation-in-python
def standard_deviation(lst, population=True):
    """Calculates the standard deviation for a list of numbers."""
    num_items = len(lst)
    mean = sum(lst) / num_items
    differences = [x - mean for x in lst]
    sq_differences = [d ** 2 for d in differences]
    ssd = sum(sq_differences)
 
    # Note: it would be better to return a value and then print it outside
    # the function, but this is just a quick way to print out the values along
    # the way.
    if population is True:
        variance = ssd / num_items
    else:
        variance = ssd / (num_items - 1)
    sd = sqrt(variance)
    return sd

data = []
for line in open('result'):
    data.append(int(line.split()[1]))

import os 
dir_path = os.path.dirname(os.path.realpath(__file__))
experiment_name = dir_path.split('/')[-1]

print experiment_name, 'mean:', mean(data), 'data size', len(data)
print experiment_name, 'standard deviation:', standard_deviation(data), 'data size', len(data)


