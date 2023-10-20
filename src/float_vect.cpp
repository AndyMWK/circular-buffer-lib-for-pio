#include "float_vect.h"


void float_vect::print() {

    if(fit_to_size()) {
        Serial.print("Entries in array: ");
        Serial.print("(");
    
        //prints in linear time
        for(int i = 0; i < numEntires; i++) {
            Serial.print(arr[i]);
            Serial.print(", ");
        }

        Serial.println(")");
    }
    
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
    if(!isnan(value)) {

        if(numEntires >= size) {
            append_right();
        }

        arr[numEntires] = value;
        numEntires++;
    }
    
}

bool float_vect::fit_to_size() {
    if(numEntires > size) {
        return false;
    }

    if(numEntires == size) {
        return true;
    }

    if(numEntires < size){
        size = numEntires;
    }

    return true;
    
}

bool float_vect::is_within_percent_treshold(float value1, float value2, float threshold) {
    float percent_diff = abs(value1 - value2)/abs((value1+value2)*0.5)*100.0;

    return percent_diff <= threshold;
}

bool float_vect::remove_values_inside_threshold(float value, float percent_threshold) {

    if(fit_to_size()) {
        if(percent_threshold > 200.0) {
            return false;
        }

        float* new_arr = new float[size];
        
        int new_arr_index = 0;
        numEntires = 0;
        for(int i = 0; i < size; i++) {

            if(!is_within_percent_treshold(value, arr[i], percent_threshold)) {
                new_arr[new_arr_index] = arr[i];
                new_arr_index++;
                numEntires++;
            }
            
        }

        delete[] arr;
        arr = new_arr;
    } else {
        return false;
    }
    

    return true;
}