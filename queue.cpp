#include "queue.h"


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

void circular_queue::enqueue(var value) {

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
var circular_queue::get_front() {
    return *(queue + ((front + 1) % size));
}

var circular_queue::get_rear() {
    return *(queue + (rear % size));
}

int circular_queue::get_size() const {
    return size;
}
 
int circular_queue::get_numEntry() const {
    return numEntries;
}

var circular_queue::get_index(int index) {

    if(index < 0 || index > size) {
        return -1;
    }

    return *(queue + index);
}

bool circular_queue::set(var value, int index) {
    if(index < 0 || index >= size) {
        return false;
    }
    
    queue[index] = value;

    return true;
}

void circular_queue::reset() {
    for(int i = 0; i < size; i++) {
        queue[i] = 0;
    }

    numEntries = 0;
    rear = 0, front = 0;
}
