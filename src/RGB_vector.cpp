#include "RGB_vector.h"
#include <Arduino.h>

using namespace RGB_vector;

float RGB_vector::calculate_vector_dist(float x1, float y1, float z1, float x0, float y0, float z0) {
    float dist = pow(x1-x0, 2) + pow(y1-y0, 2) + pow(z1-z0, 2);

    return sqrt(dist);
}

void RGB_vector::print_RGB_distance(float distance) {
    
}