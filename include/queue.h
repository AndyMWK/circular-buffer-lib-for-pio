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
    

    public: 

    circular_queue(int s) :front(0), rear(0), size(s), numEntries(0) {
        queue = new uint16_t[size];
        
        for(int i = 0; i < size; i++) {
            queue[i] = 0;
        }
    }

    ~circular_queue() {delete[] queue; Serial.println("destructor called");}

    //use for testing and troubleshooting. 
    void print_elements();

    //use for enqueue operation
    bool is_full();
    void enqueue(uint16_t &value);

    //use for dequeue operation. 
    bool is_empty();
    void dequeue();

    //use for testing and troubleshooting. 
    uint16_t get_front ();
    uint16_t get_rear ();

    uint16_t get_index(uint8_t index);

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


//function definitions for checking full queue and enqueuing. Use both functions for enqueue operations. 
bool circular_queue::is_full() {
    return numEntries >= size;
}

void circular_queue::enqueue(uint16_t &value) {
    queue[rear] = value;
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