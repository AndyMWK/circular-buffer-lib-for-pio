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

int float_vect::get_numEntry() const {
    return numEntires;
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

//find a better sorting algorithm
void float_vect::sort_ascending() {
    
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

void float_vect::reset(int s) {

    float* new_arr = new float[numEntires];

    for(int i = 0; i < numEntires; i++) {
        new_arr[i] = 0.0;
    }
    size = numEntires;

    numEntires = 0;
    
    delete[] arr;
    arr = new_arr;
}





bool float_vect::divide_into_section(float_vect &indeces) {
    if(fit_to_size()) {
        int index = 0;
        int new_arr_index = 0;
        float entry_counter = 0.0;
        float* new_arr = new float[indeces.get_numEntry() + 1];

        float sum = 0.0;
        for(int i = 0; i < numEntires; i++) {
            sum += arr[i];
            entry_counter++;

            //ISSUE HERE    
            if(i == (indeces.get_entry(index)) || i == numEntires -1) {
                new_arr[new_arr_index] = sum/entry_counter;
                sum = 0; 
                entry_counter = 0;
                index++;
                new_arr_index++;
            }
            

        }

        delete[] arr;
        arr = new_arr;
        numEntires = new_arr_index;
    } else {
        return false;
    }

    return true;
}

void float_vect::replace_with_new_array(float_vect &v) {

    float* new_arr = new float[v.get_numEntry()];

    for(int i = 0; i < v.get_numEntry(); i++) {
        new_arr[i] = v.get_entry(i);
    }

    delete[] arr;
    arr = new_arr;
}