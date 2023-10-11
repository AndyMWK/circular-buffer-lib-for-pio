#include <Arduino.h>

//circular buffer/queue implementation.
//Note: use unsigned integer 16 bits because APDS-9960 sensor returns these category of numbers. 
class circular_queue {
    private: 

    int front = 0;
    int rear = 0;
    int size;
    int numEntries = 0;
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

    float calculate_avg_float();

    void print_elements_float();

    float get_index_float(uint8_t index);

    void empty_queue_float();

};
