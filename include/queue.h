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

    float get_index_float(int index);

};

//prints all elements on the serial monitor. 
void circular_queue::print_elements() {
    Serial.println("elements in the queue: ");
    for(int i = 0; i < size; i++) {

        if(front == i) {
            Serial.print("front-> ");
        }

        if(rear == i) {
            Serial.print("rear-> ");
        } 
        Serial.print(i);
        Serial.print(" : ");
        Serial.println(*(queue + i));
    }
}

void circular_queue::print_elements_float() {
    Serial.println("elements in the queue: ");
    for(int i = 0; i < size; i++) {

        if(front == i) {
            Serial.print("front-> ");
        }

        if(rear == i) {
            Serial.print("rear-> ");
        } 
        Serial.print(i);
        Serial.print(" : ");
        Serial.println(*(queue_float + i));
    }
}


//function definitions for checking full queue and enqueuing. Use both functions for enqueue operations. 
bool circular_queue::is_full() {
    return numEntries >= size;
}

void circular_queue::enqueue(uint16_t value) {
    queue[rear] = value;
    rear = (rear + 1) % size;
    numEntries++;
}

void circular_queue::enqueue_float(float value) {
    queue_float[rear] = value;
    rear = (rear + 1) % size;
    numEntries++;
}


//function definitions for checking empty queue and dequeue. Use both functions for dequeue operation. 
bool circular_queue::is_empty() {
    return numEntries <= 0;
}

void circular_queue::dequeue() {
    front = (front + 1) % size;
    numEntries--;
}

void circular_queue::dequeue_float() {
    front = (front + 1) % size;
    numEntries--;
}


//functions for retrieving front and rear values. Use for testing. 
uint16_t circular_queue::get_front() {
    return *(queue + ((front + 1) % size));
}

uint16_t circular_queue::get_rear() {
    return *(queue + (rear % size));
}

uint16_t circular_queue::get_index(uint8_t index) {
    return *(queue + index);
}

// float circular_queue::calculate_avg_float() {

//     float sum = 0.0;
//     for(int i = 0; i < size; i++) {
//         sum += queue_float[i];

//     }
// }

float circular_queue::get_index_float(int index) {
    return *(queue_float + index);
}
