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

    if(isDyn) {
        append_right();
    }

    queue[rear] = value;
    rear = (rear + 1) % size;
    numEntries++;
}

void circular_queue::enqueue_float(float value) {

    if(isDyn) {
        append_right_float();
    }

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

    if(index < 0 || index > size) {
        return -1;
    }

    return *(queue + index);
}

float circular_queue::get_index_float(uint8_t index) {

    if(index < 0 || index > size) {
        return -1;
    }

    return *(queue_float + index);
}

void circular_queue::append_right() {
        if(numEntries >= size) {
            size *= 2;
            uint16_t* queue_new = new uint16_t[size];

            for(int i = 0; i < size; i++) {
                queue_new[i] = queue[i];
            }

            delete[] queue;

            queue = queue_new;
        }
}

void circular_queue::append_right_float() {
        if(numEntries >= size) {
            size *= 2;
            float* queue_float_new = new float[size];

            for(int i = 0; i < size; i++) {
                queue_float_new[i] = queue_float[i];
            }

            delete[] queue_float;

            queue_float = queue_float_new;
        }
}

bool circular_queue::dupe_present(uint16_t value, uint16_t deviation, bool vectorize = false) {

    if(!vectorize) {
        for(int i = 0; i < size; i++) {
        if(value <= queue[i]+deviation && value >= queue[i]-deviation) {
            return true;
        }
        }

        
    }

    if(vectorize) {
       return false;
    }
    return false;
}

int circular_queue::get_size_dyn() {
    return size;
}