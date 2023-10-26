#include "queue.h"
#include "float_vect.h"


namespace queue_helper {
    void collect_queue_data(circular_queue &q, const uint16_t value);

    void collect_queue_data_float(circular_queue &q, const float value);

    float calculate_avg(circular_queue &q, const int size); 

    float calculate_avg_float(circular_queue &q, const int size);


    //only available for floating point queue
    bool transfer_vect_data_to_queue(float_vect &f, circular_queue &q, int index);

    bool is_within_percent_treshold(float value1, float value2, float threshold);
}