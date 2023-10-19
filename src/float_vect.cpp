#include "float_vect.h"


void float_vect::print() const {

    Serial.print("Entries in array: ");
    Serial.print("(");
    
    //prints in linear time
    for(int i = 0; i < size; i++) {
        Serial.print(arr[i]);
        Serial.print(", ");
    }

    Serial.println(")");
}


int float_vect::get_size() const {
    return size;
}


int float_vect::get_entry(int index) const {

    if(index >= size || index < 0) {
        return -1;
    }
    return arr[index];
}


void float_vect::append_right() {

    size *= 2;

    float* new_arr = new float[size];

    for(int i = 0; i < numEntires; i++) {
        new_arr[i] = arr[i];
    }

    delete[] arr;
    arr = new_arr;

}

void float_vect::push_back(float value) {

    arr[numEntires] = value;

    if(numEntires >= size) {
        append_right();
    }

    numEntires++;
}