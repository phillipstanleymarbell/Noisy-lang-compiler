/*
 * http://www.analog.com/media/en/technical-documentation/technical-articles/pedometer.pdf
 *
 * This code is a hypothetical implementation of pedometer step counter as described in the paper above.
 *
 */
#include <stdio.h>
#include <stdint.h>

#define SAMPLE_SIZE 50 
#define PRECISION_THRESHOLD 0.1

enum RegulationMode { SEARCHING, FOUNDOUT };

void run_step_counter()
{
    /* initializations */
    int step_count = 0;
    int most_recent_index = 0;
    int sample_size = 0;
    time last_step_recorded = 0;
    bool array_is_full = false;
    acceleration precision_threshold = PRECISION_THRESHOLD;
    acceleration dynamic_threshold = 0;
    acceleration samples[SAMPLE_SIZE]; /* x[most_recent_index] is sample_new from the paper*/
    acceleration total = 0, threshold = 0;
    RegulationMode mode = SEARCHING;

    /* start reading from step counter */
    while (true)
    {
        acceleration sample_old = samples[most_recent_index]; /* sample_old */

        time sample_time = get_new_sample_results_and_filter(x, y, z, most_recent_index, precision_threshold);
        most_recent_index = most_recent_index + 1 % SAMPLE_SIZE;

        array_is_full = most_recent_index == 0;
        sample_size = array_is_full ? SAMPLE_SIZE : most_recent_index;

        dynamic_threshold = get_dynamic_threshold(samples, sample_size);

        if (samples[most_recent_index] < samples_old && samples[most_recent_index] < dynamic_threshold)
        {
            time step_duration = sample_time - last_recorded;

            /* time window */
            if (step_count == 0 || (step_duration > 0.2  && step_duration < 2))
            {
                step_count++;
                last_step_recorded = sample_time;

                if (step_count == 4)
                {
                    mode = FOUNDOUT;
                }
            }
            /* perform count regulation. invalid step discovered */
            else
            {
                mode = SEARCHING;
                step_count = 0;
            }
        }
    }
}

time get_new_sample_results_and_filter(
    acceleration samples[SAMPLE_SIZE], 
    int most_recent_index,
    acceleration precision_threshold
) {
    time now = read_system_clock();

    /* 
     * sample_result from the paper. Simulate adding all the inputs through a summing unit 
     */
    acceleration new_sample = read_from_x_accelerometer() + read_from_y_accelerometer() + read_from_z_accelerometer(); 

    /*
     * Here, the compiler writer will take in a list of sensors. Newton will keep check of
     * the values read for each sensor. Each sensor values will be evaluated for Newton invariant preservation 
     * at every read.
     */

    if (abs(samples[most_recent_index] - new_sample) > threshold)
        samples[most_recent_index] = new_sample;

    return now;
}

acceleration get_dynamic_threshold(
    acceleration samples[SAMPLE_SIZE], 
    int sample_size
) {
    acceleration min = LONG_MAX, max = LONG_MIN;

    for (int index = 0; index < sample_size; index ++)
    {
        if (samples[index] < min)
            min = x[index];

        if (samples[index] > max)
            max = samples[index];
    }

    return (min + max) / 2;
}

void update_average_acceleration(
    acceleration@0 samples[SAMPLE_SIZE], 
    acceleration@0 *average, 
    int sample_size
) {
    *average = 0;
    for (int index = 0; index < sample_size; index++)
    {
        *average += samples[index];
    }
    *average /= SAMPLE_SIZE;
}


