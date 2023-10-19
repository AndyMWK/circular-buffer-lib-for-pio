#include "initial_scan.h"

using namespace initial_scan;

bool initial_scan::end_scan_sequence (const float R_bck_avg, const float G_bck_avg, const float B_bck_avg, 
                            const uint16_t R, const uint16_t G, const uint16_t B, 
                            const long max_time, const int max_RGB_dist,
                            const long currentMillis) 

{
    float RGB_dist = RGB_vector::calculate_vector_dist(R_bck_avg, G_bck_avg, B_bck_avg, R, G, B);

    if(RGB_dist > max_RGB_dist) {
        return true;
    } else if(currentMillis > max_time) {
        return true;
    }

    return false;
}