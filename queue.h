#ifndef QUEUE_H
#define QUEUE_H

//circular buffer/queue implementation
#include <Arduino.h>


template<typename var>

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

    var* queue;
    
    

    public: 

    circular_queue(int s) :front(0), rear(0), size(s), numEntries(0) {
     
        queue = new var[size];
        
        for(int i = 0; i < size; i++) {
            queue[i] = 0;
        }
        
    }

    ~circular_queue() {delete[] queue;}

    //use for testing and troubleshooting. 
    void print_elements();

    //use for enqueue operation
    bool is_full();
    void enqueue(var value);

    //use for dequeue operation. 
    bool is_empty();
    void dequeue();

    var get_front();
    var get_rear();

    var get_index(int index);

    int get_size() const;
    int get_numEntry() const;

    void print_elements_float();

    bool set(var value, int index);

    void reset();
};

#endif
