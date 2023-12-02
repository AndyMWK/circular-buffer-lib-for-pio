//THESE FUNCTIONS AND CLASSES WILL NOT BE USED. IT WILL BE DELETED

#ifndef FLOAT_VECT_H
#define FLOAT_VECT_H


#include <Arduino.h>

#define DEFAULT_SIZE 4

class float_vect {
    private: 

    float* arr;
    int size;
    int numEntires;
    
    
    

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

    int get_numEntry() const;

    //gives entry at specific index
    int get_entry(int index) const;

    //appends the size of the array
    void append_right();

    //makes an entry on the array. Stores new entry at the very end. 
    void push_back(float value);

    bool remove_values_inside_threshold(float value, float percent_threshold);

    bool extract_values_oustide_threshold(float value, float percent_threshold);

    void reset(int s = DEFAULT_SIZE);

    bool divide_into_section(float_vect &indeces);

    void sort_ascending();

    bool fit_to_size();

    void replace_with_new_array(float_vect &v);

    bool is_within_percent_treshold(float value1, float value2, float threshold);

    
};

#endif
