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

# uses classification_pedometer.py

# We don't have x, y, z acceleration, 3 axis gyro data, pressure sensor data, and 3 axis magnetometer data and classifications of
# human activities of 1. sitting 2. standing, 3. walking, 4. in car
# but assume we have them. This is a hypothetical implementation of the neural net classifier 
import csv
import numpy as np
from sklearn.neural_network import MLPClassifier

INVALID = False

def get_classifier(filename):
    feature_matrix, target_values = generate_matrix_and_labels(filename)
    classifier = MLPClassifier()
    return classifier.fit(feature_matrix, target_values)


def generate_matrix_and_labels(filename):
    feature_matrix = None
    target_values = None
    with open(filename, 'rb') as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        for row in reader:
            if row:

                # check the same constraint as in pedometer.nt.c. Check that the 
                # linear velocities and the angular velocities check out, assuming that
                # the sensor data comes from a wearable on a wrist.
                # 
                # Here, there are reads from multiple sensor types in the same basic block. The generated code from
                # Python compiler will pass in these variables and Newton will perform checks on them based on
                # activity_classification_pedometer.nt
                # 
                # the values read for each sensor. Each sensor values will be evaluated for Newton invariant preservation 
                # at every read. Host lang compiler will set the global variable INVALID if invariant not satisfied.

                sample_feature_vector = np.array([
                        acceleration@0(row[3]), 
                        acceleration@1(row[4]), 
                        acceleration@2(row[5]),
                        angular_velocity@0(row[6]),
                        angular_velocity@1(row[7]),
                        angular_velocity@2(row[8]),
                        pressure(row[9]),
                        magnetic_field@0(row[10]),
                        magnetic_field@1(row[11]),
                        magnetic_field@2(row[12])
                    ])

                if feature_matrix is None:
                    feature_matrix = sample_feature_vector
                else:
                    feature_matrix.append(feature_matrix, [sample_feature_vector], axis=0)

                sample_target_values = np.array([row[2]])
                if target_values is None:
                    target_values = sample_target_values
                else:
                    target_values.append(target_values, [sample_target_values], axis=0)

    return (feature_matrix, target_values)



# read_x_y_z_data("/Users/jonathanlim/Downloads/WISDM_ar_v1.1/WISDM_ar_v1.1_raw.txt")
