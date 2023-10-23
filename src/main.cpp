#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <SPI.h>

#include <queue.h>
#include "queue_helper.h"

#include <float_vect.h>

#include "RGB_vector.h"

#include "initial_scan.h"

//Pins
#define APDS9960_INT    2  // Needs to be an interrupt pin

// Constants
#define LIGHT_INT_HIGH  100 // High light level for interrupt
#define LIGHT_INT_LOW   0   // Low light level for interrupt
#define FLOAT_POINT 2
#define uINT16 1
#define TIME_OUT 150000
#define MAX_VECT_DIST 100.0
#define DELAY_TIME 500
#define SCANNING_POLL_RATE 75
#define BCKGND_THRESHOLD 160.0
#define INCREASING 1
#define DECREASING -1
#define STABLE 0

/*
---------------GLOBAL VARIABLE DEFINITIONS---------------
*/

SparkFun_APDS9960 apds = SparkFun_APDS9960();
uint16_t ambient_light = 0;
uint16_t red_light = 0, green_light = 0, blue_light = 0;
uint16_t threshold = 0;
uint16_t integrationTime = 5;


//Queue initalization
const int arr_size = 4;

circular_queue delta_R(arr_size, FLOAT_POINT);
circular_queue delta_G(arr_size, FLOAT_POINT);
circular_queue delta_B(arr_size, FLOAT_POINT);


//PROBLEMATIC
float_vect R_profile(arr_size);
float_vect G_profile(arr_size);
float_vect B_profile (arr_size);

circular_queue R_background(arr_size, uINT16);
circular_queue G_background(arr_size, uINT16);
circular_queue B_background(arr_size, uINT16);

float_vect time_profile(arr_size);
circular_queue distance_data(arr_size, FLOAT_POINT);

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
bool initializeation_complete = false;
bool scan();

void process_scan_thresholding(float_vect &v, float bckgnd_threshold, float backgnd_avg, float color_threshold);
void extract_peak(float_vect &v, float bckgnd_threshold, float backgnd_avg);

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
  
  // Initialize APDS-9960 (configure I2C and initial values)
   if ( apds.init() ) {
    Serial.println(F("APDS-9960 initialization complete"));
  } while (!apds.init()) {
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
  if ( apds.enableLightSensor(false) ) {
    Serial.println(F("Light sensor is now running"));
  } else {
    Serial.println(F("Something went wrong during light sensor init!"));
  }
  
  // Read high and low interrupt thresholds
  if ( !apds.getLightIntLowThreshold(threshold) ) {
    Serial.println(F("Error reading low threshold"));
  } else {
    Serial.print(F("Low Threshold: "));
    Serial.println(threshold);
  }
  if ( !apds.getLightIntHighThreshold(threshold) ) {
    Serial.println(F("Error reading high threshold"));
  } else {
    Serial.print(F("High Threshold: "));
    Serial.println(threshold);
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

bool skipped_start = false;

void loop() {

  read_RGB();

  if(isr_flag) {

    if(!skipped_start) {
        skipped_start = true;
        delay(1500);
        return;
    }

    if(!initializeation_complete) {
      if(!scan()) {
        Serial.println("Scanning sequence did not complete...");
      }


      extract_peak(R_profile, BCKGND_THRESHOLD, R_background_avg);
      extract_peak(G_profile, BCKGND_THRESHOLD, G_background_avg);
      extract_peak(B_profile, BCKGND_THRESHOLD, B_background_avg);
      

      R_profile.reset(arr_size);
      G_profile.reset(arr_size);
      B_profile.reset(arr_size);

      //initializeation_complete = true;
    }


    handle_interrupt();
    //enable interrupt

  } else {
    queue_helper::collect_queue_data(R_background, red_light);
    queue_helper::collect_queue_data(G_background, green_light);
    queue_helper::collect_queue_data(B_background, blue_light);

    R_background_avg = queue_helper::calculate_avg(R_background, arr_size);
    G_background_avg = queue_helper::calculate_avg(G_background, arr_size);
    B_background_avg = queue_helper::calculate_avg(B_background, arr_size);
  }

  //print_RGB();
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



bool scan() {

  long currentMillis = millis();
  long startingMillis = millis();

  while(true) {

    read_RGB();
    

    // Serial.print("RGB: ");
    // Serial.print(red_light);
    // Serial.print(", ");
    // Serial.print(green_light);
    // Serial.print(", ");
    // Serial.print(blue_light);


    if(initial_scan::end_scan_sequence(R_background_avg, G_background_avg, B_background_avg, 
                                        red_light, green_light, blue_light, 
                                        1000, 30, currentMillis - startingMillis)) 
    {
      break;
    }

    R_profile.push_back(red_light*1.0);
    G_profile.push_back(green_light*1.0);
    B_profile.push_back(blue_light*1.0);

    time_profile.push_back(currentMillis - startingMillis);
    currentMillis = millis();

    delay(SCANNING_POLL_RATE);
  }

  return true;
}

void process_scan_thresholding(float_vect &v, float bckgnd_threshold, float bckgnd_avg, float color_threshold) {
  if(v.remove_values_inside_threshold(bckgnd_avg, bckgnd_threshold)) {
    v.print();
  }
}

void extract_peak(float_vect &v, float bckgnd_threshold, float bckgnd_avg) {
  int max_num_cycles = 10;
  if(v.extract_values_oustide_threshold(bckgnd_avg, bckgnd_threshold)) {
    v.print();
  }
  int num_cycles = 0;
  while(!v.extract_values_oustide_threshold(bckgnd_avg, bckgnd_threshold)) {
    if(num_cycles >= max_num_cycles) {
      Serial.println("error sensing color values...");
      break;
    }
  }
}


