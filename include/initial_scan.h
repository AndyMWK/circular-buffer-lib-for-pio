#include <Arduino.h>
#include <RGB_vector.h>

namespace initial_scan {

    bool end_scan_sequence (const float R_bck_avg, const float G_bck_avg, const float B_bck_avg, 
                            const uint16_t R, const uint16_t G, const uint16_t B, 
                            const long max_time, const int max_RGB_dist,
                            const long currentMillis);
}