#include "queue_helper.h"

using namespace queue_helper;

void queue_helper::collect_queue_data(circular_queue &q, const uint16_t value) {
    q.enqueue(value);

    if(!q.is_full()) {

        return;
    }

    if(!q.is_empty()) {
        q.dequeue();
    }
}

void queue_helper::collect_queue_data_float(circular_queue &q, const float value) {
    q.enqueue_float(value);

    if(!q.is_full()) {

        return;
    }

    if(!q.is_empty()) {
        q.dequeue_float();
    }
}

float queue_helper::calculate_avg(circular_queue &q, const int size) {
    float sum = 0;
    for(int i = 0; i < size; i++) {
        sum += q.get_index(i);
    }

    return sum/size;
}

float queue_helper::calculate_avg_float(circular_queue &q, const int size) {
    float sum = 0;
    for(int i = 0; i < size; i++) {
        sum += q.get_index_float(i);
     }

    return sum/size;
}