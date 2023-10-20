#ifndef FLOAT_VECT_H
#define FLOAT_VECT_H


#include <Arduino.h>

class float_vect {
    private: 

    float* arr;
    int size;
    int numEntires;
    
    bool fit_to_size();
    bool is_within_percent_treshold(float value1, float value2, float threshold);

    public: 

    //Constructor
    float_vect(int s) : size(2*s + 1), numEntires(0) {
        if(s < 0 ) {
            Serial.println("Initial size of the float vector must be greater than 0");
        } else {
            arr = new float[size];

            for(int i = 0; i < size; i++) {
                arr[i] = 0.0;
            }
        }
    }

    //default constructor to be used for delcarations
    float_vect() : float_vect(0) {}

    //destructor
    ~float_vect() {
        delete[] arr;
    }

    //printing values
    void print();

    //gives size
    int get_size() const;

    //gives entry at specific index
    int get_entry(int index) const;

    //appends the size of the array
    void append_right();

    //makes an entry on the array. Stores new entry at the very end. 
    void push_back(float value);

    bool remove_values_inside_threshold(float value, float percent_threshold);

};

#endif