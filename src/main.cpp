#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <SPI.h>

//dependencies to help queue functionality
#include <queue.h>
#include "queue_helper.h"

//dynamic float array
#include "float_vect.h"

//includes Euclidian Space Functions for RGB
#include "RGB_vector.h"

//Pins
#define APDS9960_INT    2  // Needs to be an interrupt pin

// Constants
#define LIGHT_INT_HIGH  100 // High light level for interrupt
#define LIGHT_INT_LOW   0   // Low light level for interrupt
#define FLOAT_POINT 2
#define uINT16 1
#define DELAY_TIME 100
#define SCANNING_POLL_RATE 50
#define RGB_dist_perct_threshold
#define SCAN_TIME 1000
#define NUM_PROFILES 2
#define RGB_dist_threshold 300


/*
---------------GLOBAL VARIABLE DEFINITIONS---------------
*/

SparkFun_APDS9960 apds = SparkFun_APDS9960();
uint16_t ambient_light = 0;
uint16_t red_light = 0, green_light = 0, blue_light = 0;
uint16_t threshold = 0;
uint16_t integrationTime = 5;


//Queue initalization
const int arr_size = SCAN_TIME/DELAY_TIME;
const int background_arr_size = 4;

circular_queue R_background(background_arr_size, uINT16);
circular_queue G_background(background_arr_size, uINT16);
circular_queue B_background(background_arr_size, uINT16);

circular_queue collected_profile_R(arr_size, FLOAT_POINT);
circular_queue collected_profile_G(arr_size, FLOAT_POINT);
circular_queue collected_profile_B(arr_size, FLOAT_POINT);

circular_queue pulse_avg_R(NUM_PROFILES, FLOAT_POINT);
circular_queue pulse_avg_G(NUM_PROFILES, FLOAT_POINT);
circular_queue pulse_avg_B(NUM_PROFILES, FLOAT_POINT);


/*
---------------FUNCITON DECLARATIONS---------------
*/


//interrupt service routine
volatile bool isr_flag = false;
void interruptRoutine();
void handle_interrupt();


//Serial Monitor Interface 
void read_RGB();
void print_RGB();
float R_background_avg = 0.0, G_background_avg = 0.0, B_background_avg = 0.0;

//scanning initialization
bool scan_complete = false;
bool scan(float_vect &R, float_vect &G, float_vect &B);

void process_scan_thresholding(float_vect &v, float bckgnd_threshold, float backgnd_avg, float color_threshold);

bool divide_profiles(float_vect &R, float_vect &G, float_vect &B, int size, int &profile_division_index);

void enable_apds();

long startingMillis = 0;
void setup() {
  
  // Set interrupt pin as input
  pinMode(APDS9960_INT, INPUT);
  
  // Initialize Serial port
  Serial.begin(9600);
  Serial.println();
  Serial.println(F("-------------------------------------"));
  Serial.println(F("APDS 9960 - Colour Consistency Sensing"));
  Serial.println(F("-------------------------------------"));
  
  // Initialize interrupt service routine
  attachInterrupt(0, interruptRoutine, FALLING);
  
  enable_apds();

  startingMillis = millis();
 
}

bool skipped_start = false;

bool scan2(float_vect &R, float_vect &G, float_vect &B) {

  long currentMillis = millis();
  

  if(currentMillis - startingMillis < SCAN_TIME) {

    read_RGB();

    R.push_back(red_light*1.0);
    G.push_back(green_light*1.0);
    B.push_back(blue_light*1.0);
  }

  return true;
}

int data_collected = 0;

void loop() {

  read_RGB();

  if(isr_flag) {

    handle_interrupt();
    //apds.disableLightSensor();
    
    if(!skipped_start) {
        skipped_start = true;
        delay(1500);
        return;
    }

    //collection state
    if(data_collected < arr_size) {
      queue_helper::collect_queue_data_float(collected_profile_R, red_light);
      queue_helper::collect_queue_data_float(collected_profile_G, green_light);
      queue_helper::collect_queue_data_float(collected_profile_B, blue_light);

      data_collected++;

    } 
    

    //should be at the processing state
    else if(data_collected == arr_size) {
      Serial.println("------RGB Array------");
      collected_profile_R.print_elements_float();
      collected_profile_G.print_elements_float();
      collected_profile_B.print_elements_float();

      circular_queue temp_arr_R(arr_size, FLOAT_POINT);

      int mode = 1;
      
      float prev_R = 0.0;
      float prev_G = 0.0;
      float prev_B = 0.0;

      for(int i = 0; i < arr_size; i++) {
        if(mode == 1) {
          float RGB_dist = RGB_vector::calculate_vector_dist(R_background_avg, G_background_avg, B_background_avg,
                                                              collected_profile_R.get_index_float(i), 
                                                              collected_profile_G.get_index_float(i), 
                                                              collected_profile_B.get_index_float(i));

          if(!queue_helper::is_within_percent_treshold(RGB_dist, 0.0, 170.0)) {
            //Serial.println("j");
            queue_helper::collect_queue_data_float(temp_arr_R, collected_profile_R.get_index_float(i));
            mode = 2;
          }
        }

        else if(mode == 2) {
          float RGB_dist = RGB_vector::calculate_vector_dist(prev_R, prev_G, prev_B,
                                                              collected_profile_R.get_index_float(i), 
                                                              collected_profile_G.get_index_float(i), 
                                                              collected_profile_B.get_index_float(i));

          float RGB_dist_from_gnd = RGB_vector::calculate_vector_dist(R_background_avg, G_background_avg, B_background_avg,
                                                              collected_profile_R.get_index_float(i), 
                                                              collected_profile_G.get_index_float(i), 
                                                              collected_profile_B.get_index_float(i));

          if(queue_helper::is_within_percent_treshold(RGB_dist, 0.0, 10.0)) {
            //Serial.println("h");
            queue_helper::collect_queue_data_float(temp_arr_R, collected_profile_R.get_index_float(i));
          } 
          
          else if(queue_helper::is_within_percent_treshold(RGB_dist_from_gnd, RGB_dist, 170)) {
            mode = 1;
          }
         
         //use hash map..?
        }
        
        

        prev_R = collected_profile_R.get_index_float(i);
        prev_G = collected_profile_G.get_index_float(i);
        prev_B = collected_profile_B.get_index_float(i);
      }
      
      Serial.println("-----temp_r-----");
      temp_arr_R.print_elements_float();
      data_collected = 0;
    }
    
    //enable_apds();

  } else {
    queue_helper::collect_queue_data(R_background, red_light);
    queue_helper::collect_queue_data(G_background, green_light);
    queue_helper::collect_queue_data(B_background, blue_light);

    R_background_avg = queue_helper::calculate_avg(R_background, background_arr_size);
    G_background_avg = queue_helper::calculate_avg(G_background, background_arr_size);
    B_background_avg = queue_helper::calculate_avg(B_background, background_arr_size);
    print_RGB();
  }

  
  delay(DELAY_TIME);
}


/*
---------------FUNCTION DEFINITIONS---------------
*/


void interruptRoutine() {
  isr_flag = true;
}

void handle_interrupt() {
  isr_flag = false;
  if( !apds.clearAmbientLightInt() ) {
    Serial.println("Error clearing interrupt");
  }
}

void print_RGB() {
  Serial.print("RGB: ");
  Serial.print(R_background_avg);
  Serial.print(", ");
  Serial.print(G_background_avg);
  Serial.print(", ");
  Serial.print(B_background_avg);

  Serial.print("    C: ");
  Serial.println(ambient_light);
}

void read_RGB() {

  /*
  Read the light levels (ambient, red, green, blue)
  do not enter this loop when interrupt is handled.
  */ 

  while (  !apds.readAmbientLight(ambient_light) ||
        !apds.readRedLight(red_light) ||
        !apds.readGreenLight(green_light) ||
        !apds.readBlueLight(blue_light) ) 
  {
    Serial.println("Error reading light values");
  }

}



bool scan(float_vect &R, float_vect &G, float_vect &B) {

  long currentMillis = millis();
  long startingMillis = millis();

  while(currentMillis - startingMillis < SCAN_TIME) {

    read_RGB();

    R.push_back(red_light*1.0);
    G.push_back(green_light*1.0);
    B.push_back(blue_light*1.0);

    //Serial.println(R.get_numEntry());
    currentMillis = millis();

    delay(SCANNING_POLL_RATE);
  }

  

  if(R.get_numEntry() != G.get_numEntry() || R.get_numEntry() != B.get_numEntry() ||
      G.get_numEntry() != B.get_numEntry()) 
  {
      
      return false;
  }

  return true;
}

void process_scan_thresholding(float_vect &v, float bckgnd_threshold, float bckgnd_avg, float color_threshold) {
  if(v.remove_values_inside_threshold(bckgnd_avg, bckgnd_threshold)) {
    v.print();
  }
}


bool divide_profiles(float_vect &R, float_vect &G, float_vect &B, int size, int &profile_division_index) {
  float max_RGB_dist = 0.0;
  for(int i = 0; i < size-1; i++) {
    float RGB_dist = RGB_vector::calculate_vector_dist(R.get_entry(i), G.get_entry(i), B.get_entry(i),
                                                          R.get_entry(i+1), G.get_entry(i+1), B.get_entry(i+1));

    if(RGB_dist > max_RGB_dist) {
      max_RGB_dist = RGB_dist;
      profile_division_index = i;
    }
    
  } 

  if(max_RGB_dist == 0.0) {
    return false;
  } else {
    return true;
  }
}

void enable_apds() {
   // Initialize APDS-9960 (configure I2C and initial values)
   if (!apds.init()) {
    Serial.println(F("Something went wrong during APDS-9960 init!"));
  }

  // Set high and low interrupt thresholds
  while ( !apds.setLightIntLowThreshold(LIGHT_INT_LOW) ) {
    Serial.println(F("Error writing low threshold"));
  }
  while ( !apds.setLightIntHighThreshold(LIGHT_INT_HIGH) ) {
    Serial.println(F("Error writing high threshold"));
  }
  
  // Start running the APDS-9960 light sensor (no interrupts)
  if (!apds.enableLightSensor(false) ) {
    Serial.println(F("Something went wrong during light sensor init!"));
  }
  
  // Read high and low interrupt thresholds
  if ( !apds.getLightIntLowThreshold(threshold) ) {
    Serial.println(F("Error reading low threshold"));
  } 
  if ( !apds.getLightIntHighThreshold(threshold) ) {
    Serial.println(F("Error reading high threshold"));
  } 
  
  // Enable interrupts
  while ( !apds.setAmbientLightIntEnable(1) ) {
    Serial.println(F("Error enabling interrupts"));
  }
  
  //change the time the diodes are exposed to light. Lower integration time means less sensitivity. 
  while ( !apds.setADCIntegrationTime(integrationTime)) {
    Serial.println("Integration Time not set...");
  }

  // Wait for initialization and calibration to finish
  
  delay(DELAY_TIME);
}