/*
  Pedometer implementation based on [1].

	Authored 2020,	Orestis Kaparounakis.

  Copyright (c) 2020, Orestis Kaparounakis.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

	[1] N. Zhao, “Full-Featured Pedometer Design Realized with 3-Axis
	Digital Accelerometer,” p. 5, 2010.
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <printf.h>
#include <string.h>
#include <math.h>

#define SAMPLES_PER_MEASUREMENT 4
#define DC_SAMPLES_FACTOR (12)
#define PRECISION (0.5)

typedef float bmx055xAcceleration;
typedef float bmx055yAcceleration;
typedef float bmx055zAcceleration;

/*
	Accelerometer samples at 400 Hz.
	Humans walk with frequency 1 Hz to 5 Hz.
	We need acceleration in the direction of gravity.
	From Nyquist theorem we need to sample at >10 Hz.
	If sampling at 10 Hz, we get 40 samples from acc.
	At 20 Hz -> 20 acc samples.
	At 40 Hz -> 10 acc samples.
 */
int main(int argc, char *argv[])
{
	char	charBuffer[150];
	FILE *	inputFile;

//	printf("Input filename: %s\n", argv[1]);

	inputFile = fopen(argv[1], "r");
	if (inputFile == NULL)
	{
		fprintf(stderr, "Error opening input file.\n");
		exit(1);
	}

	/*
		Ignore labels of first .csv line
	 */
	while (fgetc(inputFile) != '\n');
//	printf("Reading from file...\n");

	int reachedEOF = 0;

	int measurementCount = -1;
	int DCEstimateCounter = 0; // Counter for zDC

	int reset = 1;
	int steps = 0;
	int strides = 0;
	int inStride = 1;

	float stepProbability = 0.0;
	float stepIntersectionProbability;
	float fSteps = 0.0;
	float bernoulliSum = 0.0;
	float poissonBinomialAccum = 0.0;

	float timestamp;
	float timestampPrevious = -1000.0;
	float fTimestampPrevious;

	/*
		Arrays for samples.
	 */
	float timestampSamples[SAMPLES_PER_MEASUREMENT];
	float xSamples[SAMPLES_PER_MEASUREMENT];
	float ySamples[SAMPLES_PER_MEASUREMENT];
	float zSamples[SAMPLES_PER_MEASUREMENT];
	float xDCSamples[DC_SAMPLES_FACTOR * SAMPLES_PER_MEASUREMENT];
	float yDCSamples[DC_SAMPLES_FACTOR * SAMPLES_PER_MEASUREMENT];
	float zDCSamples[DC_SAMPLES_FACTOR * SAMPLES_PER_MEASUREMENT];

	bmx055xAcceleration acc_x = 3.4;
	bmx055yAcceleration acc_y = 3.4;
	bmx055zAcceleration acc_z = 3.4;

	float max_x = -10000;
	float max_y = -10000;
	float max_z = -10000;
	float min_x = 10000;
	float min_y = 10000;
	float min_z = 10000;
	float threshold_x = 0;
	float threshold_y = 0;
	float threshold_z = 0;

	bmx055xAcceleration measurementOld_x = 1.2;
	bmx055yAcceleration measurementOld_y = 1.2;
	bmx055zAcceleration measurementOld_z = 1.2;
	bmx055xAcceleration measurementNew_x = 1.2;
	bmx055yAcceleration measurementNew_y = 1.2;
	bmx055zAcceleration measurementNew_z = 1.2;

	float xDC = 0;
	float yDC = 0;
	float zDC = 0;

	float zRange;

//	printf("Performing measurements...\n");

	struct timeval tv;                                                     \
    gettimeofday(&tv, (void *)0);                                          \
    double t1 = tv.tv_sec + 1.0e-6 * tv.tv_usec;

	while (!reachedEOF)
	{
		/*
			MEASUREMENT SECTION
		 */
		/*
			Perform a measurement by "sampling" the
			accelerometer $SAMPLES_PER_MEASUREMENT times.
		 */
		float	timestampAccum = 0.0;
		int	samplesTaken = 0;
		// printf("time\t\tacc_x\t\tacc_y\t\tacc_z\n");
		acc_x = 0;
		acc_y = 0;
		acc_z = 0;        
		for (size_t i = 0; i < SAMPLES_PER_MEASUREMENT; i++)
		{
			// int itemsRead = fscanf(inputFile, "%G,%G,%G,%G\n", &timestampSamples[i], &xSamples[i], &ySamples[i], &zSamples[i]);
			int itemsRead = fscanf(inputFile, "%G,%G,%G,%G,%*G\n", &timestampSamples[i], &xSamples[i], &ySamples[i], &zSamples[i]);
			if (itemsRead == EOF || itemsRead == 0)
			{
				/*
					Hopefully just the End Of File.
				*/
				reachedEOF = 1;
				break;
			}
			else if (itemsRead != 4)
			{
				fprintf(stderr, "Unable to read row. Possible problem with input data format.\n");
			}
			// printf("%G,%G,%G,%G\n", timestampSamples[i], xSamples[i], ySamples[i], zSamples[i]);
			timestampAccum += timestamp;
			samplesTaken++;

			/*
				Smoothen (Figure 4)
			*/
			acc_x += xSamples[i];
			acc_y += ySamples[i];
			acc_z += zSamples[i];
			timestamp = timestampSamples[i];
		}

		// acc_x /= SAMPLES_PER_MEASUREMENT;
		// acc_y /= SAMPLES_PER_MEASUREMENT;
		// acc_z /= SAMPLES_PER_MEASUREMENT;

		/*
			Optionally combine all axes together.
		 */
		// acc_z = acc_z + acc_x + acc_y;

		/*
			PROCESSING SECTION
		 */

		float diff_z;

		measurementOld_x = measurementNew_x;
		measurementOld_y = measurementNew_y;
		measurementOld_z = measurementNew_z;

		if (measurementCount == -1)
		{
			measurementNew_z = acc_y;
			measurementCount = 0;
		}

		/*
			Uncomment if every sensor measurement is a measurement.
			See below.
		 */
		// measurementNew_x = acc_x;
		// measurementNew_y = acc_y;
		// measurementNew_z = acc_z;

		/*
			The next part essentially skips measurements that are not
			significantly different the previous ones. The reasoning
			for this (in classical) algorithms is that it smooths the
			measurements curve [1].

			In our case we should also examine the performance of the
			algorithm without this smoothing as it includes extra
			distributional information, which the distributional
			variables could potentially encode.
		 */
		/*
			Check if change is significant
		 */
		if (fabsf(acc_y - measurementNew_z) > PRECISION)
		{
			measurementNew_z = acc_y;
			// printf("New measurement is %f.\n", measurementNew_z);
		}
		else
		{
			/*
				Z-axis acceleration change was not
				significant enough to register it.
			 */
			// printf("Change not significant:  %f -> %f.\n", measurementNew_z, acc_y);
			continue;
		}

		/*
			Update maximum and minimum values for Z-axis.
		 */
		if (measurementNew_z > max_z)
		{
			// printf("New max is %f\n", measurementNew_z);
			max_z = measurementNew_z;
		}
		if (min_z > measurementNew_z)
		{
			// printf("New min is %f\n", measurementNew_z);
			min_z = measurementNew_z;
		}
		/*
			Update threshold value for Z-axis every 30 measurements.
		 */
		zRange = max_z - min_z;
		// printf("zRange=%f\n", zRange);
		if (measurementCount < 10 || measurementCount % 50)
		{
			threshold_z = (max_z + min_z) / 2;

			/*
				"Reset" min and max so they change at next
				iteration.
			 */
			float temp_z;
			temp_z = max_z;
			max_z = min_z;
			min_z = temp_z;
			// printf("New threshold is %f.\n", threshold_z);
		}

		/*
			Deterministic steps.
		 */
		if (measurementOld_z > measurementNew_z
		    	&&	threshold_z > measurementNew_z
			&&	timestamp - timestampPrevious > 0.3
			&&	zRange > 2 && zRange < 8)
		{
			steps++;
			reset = 0;
			timestampPrevious = timestamp;
			// printf("Deterministic step registered at %f. Step count: %d.\n", timestamp, steps);
		}

		if (timestamp - timestampPrevious > 2 && reset == 0)
		{
			/*
				Finished a stride.
			 */
			// printf("Stride #%d steps: %d\n", strides, steps);
			// steps = 0;
			strides++;
			reset = 1;
		}

		measurementCount++;
	}

	gettimeofday(&tv, (void *)0);                                          \
    double t2 = tv.tv_sec + 1.0e-6 * tv.tv_usec;

	printf("%lf\n", t2 - t1);
//	printf("They took %d steps, %d strides.\n", steps, strides);
//	printf("Measurement count total : %d.\n", measurementCount);

	fclose(inputFile);

	return 0;
}
