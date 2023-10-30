#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <SPI.h>

//dependencies to help queue functionality
#include <queue.h>
#include "queue_helper.h"

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
#define BCKGND_THRESH 180
#define SCAN_TIME 1000
#define NUM_PROFILES 2
#define SAMPLE_SIZE 5



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
const int background_arr_size = 6;
const float origin_x = 0.0, origin_y = 0.0, origin_z = 0;

circular_queue R_background(background_arr_size, uINT16);
circular_queue G_background(background_arr_size, uINT16);
circular_queue B_background(background_arr_size, uINT16);

circular_queue collected_profile_R(arr_size, FLOAT_POINT);
circular_queue collected_profile_G(arr_size, FLOAT_POINT);
circular_queue collected_profile_B(arr_size, FLOAT_POINT);

circular_queue pulse_avg_R(SAMPLE_SIZE, FLOAT_POINT);
circular_queue pulse_avg_G(SAMPLE_SIZE, FLOAT_POINT);
circular_queue pulse_avg_B(SAMPLE_SIZE, FLOAT_POINT);


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

void enable_apds();

float calculate_std(float* arr, int size);
bool RGB_consistent(float max_std);

bool process_pulse_avg();

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
}

bool skipped_start = false;
bool process_data = false;
int data_collected = 0;
int pulse_avg_processed = 0;

void loop() {

  read_RGB();
  if(!isr_flag) {
    queue_helper::collect_queue_data(R_background, red_light);
    queue_helper::collect_queue_data(G_background, green_light);
    queue_helper::collect_queue_data(B_background, blue_light);

    R_background_avg = queue_helper::calculate_avg(R_background, background_arr_size);
    G_background_avg = queue_helper::calculate_avg(G_background, background_arr_size);
    B_background_avg = queue_helper::calculate_avg(B_background, background_arr_size);
    print_RGB();
  }

  else if(isr_flag) {

    handle_interrupt();
    
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
    
    //preprocessing state
    else if(data_collected == arr_size) {
      Serial.println("------RGB Array------");

      collected_profile_R.print_elements_float();
      collected_profile_G.print_elements_float();
      collected_profile_B.print_elements_float();
      
      if(process_pulse_avg()) {
        pulse_avg_processed++;
        
      }

      //transition to collection state
      data_collected = 0;
      collected_profile_R.reset();
      collected_profile_G.reset();
      collected_profile_B.reset();
     
    }
    
    //transition to processing state
    if(pulse_avg_processed == SAMPLE_SIZE) {
      Serial.println("---Pulse Average---");
      pulse_avg_R.print_elements_float();
      pulse_avg_G.print_elements_float();
      pulse_avg_B.print_elements_float();
      process_data = true;
    }
    
    //processing state
    if(process_data) {
      //disable apds
      if(RGB_consistent(30.0)) {
        Serial.println("Test Ended...");
        process_data = false;
        pulse_avg_processed = 0;
        data_collected  = 0;

        pulse_avg_R.reset();
        pulse_avg_G.reset();
        pulse_avg_B.reset();
        //reset array...
      }

      delay(DELAY_TIME * 10);
    }

    
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

float calculate_std(float* arr, int size) {
  float sum = 0.0;
  Serial.println("----RGB Distance----");
  for(int i = 0; i < size; i++) {
    Serial.print("Avg dist: ");
    Serial.println(arr[i]);
    sum += arr[i];
  }
  float avg = sum/(size*1.0);

  float top = 0.0;
  for(int i = 0; i < size; i++) {
    top += pow(arr[i] - avg, 2);
  }

  return sqrt(top/(size - 1)*1.0);
}

bool RGB_consistent(float max_std) {

  float* arr = new float[SAMPLE_SIZE];

  for(int i = 0; i < SAMPLE_SIZE; i++) {
    arr[i] = RGB_vector::calculate_vector_dist(pulse_avg_R.get_index_float(i), pulse_avg_G.get_index_float(i), 
                                                pulse_avg_B.get_index_float(i), origin_x, origin_y, origin_z);
  }

  float std = calculate_std(arr, SAMPLE_SIZE);

  Serial.print("STD: ");
  Serial.println(std);

  delete[] arr;
  return true;
}

bool process_pulse_avg() {

  float prev_R = collected_profile_R.get_index_float(0);
      
  float total_sum_R = 0;
  float total_sum_G = 0;
  float total_sum_B = 0;

  int how_many_collected = 0;

  for(int i = 0; i < arr_size; i++) {

          prev_R = collected_profile_R.get_index_float(i);
        
          if(queue_helper::is_within_percent_treshold(collected_profile_R.get_index_float(i), R_background_avg, 120)) {
          
            continue;
          }

          if(queue_helper::is_within_percent_treshold(collected_profile_R.get_index_float(i), prev_R, 50.0)) {
          
            total_sum_R += collected_profile_R.get_index_float(i);
            total_sum_G += collected_profile_G.get_index_float(i);
            total_sum_B += collected_profile_B.get_index_float(i);

            how_many_collected++;
          }
    }

    Serial.println("----Collected Sum----");
    Serial.print("R: ");
    Serial.print(total_sum_R);
    Serial.print("  G: ");
    Serial.print(total_sum_G);
    Serial.print("  B: ");
    Serial.println(total_sum_B);

    Serial.println("----# of collected data points----");
    Serial.print("#: ");
    Serial.println(how_many_collected);

    if(how_many_collected == 0) {
      return false;
    }

    queue_helper::collect_queue_data_float(pulse_avg_R, total_sum_R/how_many_collected);
    queue_helper::collect_queue_data_float(pulse_avg_G, total_sum_G/how_many_collected);
    queue_helper::collect_queue_data_float(pulse_avg_B, total_sum_B/how_many_collected);

    return true;
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
  delay(DELAY_TIME*10);
}