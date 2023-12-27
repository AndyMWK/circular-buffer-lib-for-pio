#include "queue.h"


//prints all elements on the serial monitor. 
template<typename var>
void circular_queue<var>::print_elements() {
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
template<typename var> 
bool circular_queue<var>::is_full() {
    return numEntries >= size;
}

template<typename var> 
void circular_queue<var>::enqueue(var value) {

    queue[rear] = value;
    rear = (rear + 1) % size;
    numEntries++;
}

//function definitions for checking empty queue and dequeue. Use both functions for dequeue operation. 
template<typename var> 
bool circular_queue<var>::is_empty() {
    return numEntries <= 0;
}

template<typename var> 
void circular_queue<var>::dequeue() {
    front = (front + 1) % size;
    numEntries--;
}

//functions for retrieving front and rear values. Use for testing. 
template<typename var> 
var circular_queue<var>::get_front() {
    return *(queue + ((front + 1) % size));
}

template<typename var> 
var circular_queue<var>::get_rear() {
    return *(queue + (rear % size));
}

template<typename var> 
int circular_queue<var>::get_size() const {
    return size;
}

template<typename var> 
int circular_queue<var>::get_numEntry() const {
    return numEntries;
}

template<typename var> 
var circular_queue<var>::get_index(int index) {

    if(index < 0 || index > size) {
        return -1;
    }

    return *(queue + index);
}


template<typename var> 
bool circular_queue<var>::set(var value, int index) {
    if(index < 0 || index >= size) {
        return false;
    }
    
    queue[index] = value;

    return true;
}

template<typename var> 
void circular_queue<var>::reset() {
    for(int i = 0; i < size; i++) {
        queue[i] = 0;
    }

    numEntries = 0;
    rear = 0, front = 0;
}

template class circular_queue<uint16_t>;
template class circular_queue<float>;
