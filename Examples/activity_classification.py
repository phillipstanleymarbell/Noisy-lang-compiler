# This data has been released by the Wireless Sensor Data Mining
# (WISDM) Lab. <http://www.cis.fordham.edu/wisdm/>
# 
# The data in this file corrispond with the data used in the
# following paper:
# 
#     Jennifer R. Kwapisz, Gary M. Weiss and Samuel A. Moore (2010).
#     Activity Recognition using Cell Phone Accelerometers,
#     Proceedings of the Fourth International Workshop on Knowledge
#     Discovery from Sensor Data (at KDD-10), Washington DC.
#     <http://www.cis.fordham.edu/wisdm/public_files/sensorKDD-2010.pdf>

import csv


def read_x_y_z_data(filename):
    data_to_tag = {}
    with open(filename, 'rb') as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        for row in reader:
            # print row
            if row and row[3] and row[4] and row[5] and row[2]:
                z = 0
                if ';' in row[5]:
                    z = float(row[5].replace(';', ''))
                else:
                    z = float(row[5])
                data_to_tag[(float(row[3]), float(row[4]), float(z))] = row[2]

    return data_to_tag

read_x_y_z_data("/Users/jonathanlim/Downloads/WISDM_ar_v1.1/WISDM_ar_v1.1_raw.txt")

