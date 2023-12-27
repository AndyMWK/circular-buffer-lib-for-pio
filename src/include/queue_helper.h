#include "queue.h"

namespace queue_helper {
    
    template<typename var>
    void collect_queue_data(circular_queue<var> &q, const var value) {
        q.enqueue(value);

        if(!q.is_full()) {

            return;
        }

        if(!q.is_empty()) {
            q.dequeue();
        }
    }

    template<typename var>
    float calculate_avg(circular_queue<var> &q, const int size) {
        var sum = 0;
        for(int i = 0; i < size; i++) {
            sum += q.get_index(i);
        }

        return sum/size;
    }

    bool is_within_percent_treshold(float value1, float value2, float threshold) {
        float percent_diff = abs(value1 - value2)/abs((value1+value2)*0.5)*100.0;

        return percent_diff <= threshold;
    }
}
