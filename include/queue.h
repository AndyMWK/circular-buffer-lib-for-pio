#ifndef QUEUE_H
#define QUEUE_H



#include <Arduino.h>

//circular buffer/queue implementation.
//Note: use unsigned integer 16 bits because APDS-9960 sensor returns these category of numbers. 
class circular_queue {
    private: 

    //Index references
    int front = 0;
    int rear = 0;

    //queue capacity/size
    int size;

    //current number of entries in the queue
    int numEntries = 0;

    //integer or floating point queue delcarations. 
    //You can set them to either calculate only integers or floating points
    uint16_t* queue;
    float* queue_float;
    

    public: 

    circular_queue(int s, int mode) :front(0), rear(0), size(s), numEntries(0) {
        if(mode == 1) {
            queue = new uint16_t[size];
        
            for(int i = 0; i < size; i++) {
                queue[i] = 0;
            }
        }

        if(mode == 2) {
            queue_float = new float[size];

            for(int i = 0; i < size; i++) {
                
                queue_float[i] = 0;
            }
        }
        
    }

    ~circular_queue() {delete[] queue; delete[] queue_float;}

    //use for testing and troubleshooting. 
    void print_elements();

    //use for enqueue operation
    bool is_full();
    void enqueue(uint16_t value);
    void enqueue_float(float value);

    //use for dequeue operation. 
    bool is_empty();
    void dequeue();
    void dequeue_float();

    //use for testing and troubleshooting. 
    uint16_t get_front ();
    uint16_t get_rear ();

    uint16_t get_index(uint8_t index);

    int get_size() const;
    int get_numEntry() const;

    float calculate_avg_float();

    void print_elements_float();

    float get_index_float(uint8_t index);


    bool dupe_present(uint16_t value, uint16_t deviation, bool vectorize);

    void collect_queue_data(const uint16_t value);

    void collect_queue_data_float(const float value);

    float calculate_avg(const int size);

    float calculate_avg_float(const int size);

    bool is_within_percent_treshold(float value1, float value2, float threshold);

    bool set(float value, int index);
};

#endif