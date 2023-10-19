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
#define LIGHT_INT_HIGH  200 // High light level for interrupt
#define LIGHT_INT_LOW   0   // Low light level for interrupt
#define FLOAT_POINT 2
#define uINT16 1
#define COLD 1
#define WARM 2
#define DYNAMIC_TRUE 1
#define DYNAMIC_FALSE 0
#define TIME_OUT 150000
#define MAX_VECT_DIST 100.0
#define DELAY_TIME 100
#define SCANNING_POLL_RATE 25

/*
---------------GLOBAL VARIABLE DEFINITIONS---------------
*/

SparkFun_APDS9960 apds = SparkFun_APDS9960();
uint16_t ambient_light = 0;
uint16_t red_light = 0, green_light = 0, blue_light = 0;
uint16_t threshold = 0;
uint16_t integrationTime = 25;


//Queue initalization
const int arr_size = 4;

circular_queue delta_R(arr_size, FLOAT_POINT);
circular_queue delta_G(arr_size, FLOAT_POINT);
circular_queue delta_B(arr_size, FLOAT_POINT);

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

void setup() {
  
  // Set interrupt pin as input
  pinMode(APDS9960_INT, INPUT);
  
  // Initialize Serial port
  Serial.begin(9600);
  Serial.println();
  Serial.println(F("-------------------------------------"));
  Serial.println(F("APDS 9960 - Light Sensing"));
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



void loop() {

  read_RGB();

  if(isr_flag) {
    //disable interrupt
    if(!initializeation_complete) {
      if(!scan()) {
        Serial.println("Scanning sequence did not complete...");
      }

      R_profile.print();
      G_profile.print();
      B_profile.print();
      
      initializeation_complete = true;
    }
    

    //process RGB values
    //reset initiial scanning once a conclusion has been reached 


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
  Serial.print(red_light);
  Serial.print(", ");
  Serial.print(green_light);
  Serial.print(", ");
  Serial.print(blue_light);

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

  bool read = true;
  long currentMillis = millis();
  long startingMillis = millis();

  while(read) {
    read_RGB();
    R_profile.push_back(red_light);
    G_profile.push_back(green_light);
    B_profile.push_back(blue_light);

    time_profile.push_back(currentMillis - startingMillis);
    currentMillis = millis();

    if(initial_scan::end_scan_sequence(R_background_avg, G_background_avg, B_background_avg, 
                                        red_light, green_light, blue_light, 2000, 30, currentMillis)) 
    {
      read = false;
    }

    delay(SCANNING_POLL_RATE);
  }

  return true;
}


// uint16_t distance_threshold = 50;

// bool dupe_present_vect(circular_queue &qx, circular_queue &qy, circular_queue &qz, 
//                         float threshold, int array_size, uint16_t x, uint16_t y, uint16_t z) 
// {
//   for(int i = 0; i < array_size; i++) {
//     float vect_dist = calculate_vector_dist(qx.get_index(i), qy.get_index(i), qz.get_index(i), x, y, z);

//     if(vect_dist <= threshold) {
//       return true;
//     }
//   }

//   return false;
// }

// //need to account for the background
// bool initial_scanning() {
  
//   bool read = true;
  

//   uint16_t profile_isntance_R = 0, profile_instance_G = 0, profile_istance_B = 0;

//   // float R_back_avg = calculate_avg(R_background, arr_size);
//   // float G_back_avg = calculate_avg(G_background, arr_size);
//   // float B_back_avg = calculate_avg(B_background, arr_size);

//   apds.readRedLight(profile_isntance_R);
//   apds.readGreenLight(profile_instance_G);
//   apds.readBlueLight(profile_istance_B);

//   collect_queue_data(R_profile, profile_isntance_R);
//   collect_queue_data(G_profile, profile_instance_G);
//   collect_queue_data(B_profile, profile_istance_B);

//   long current_millis = millis();

//   long prev_millis = millis();

//   Serial.println("Profile Instance Values...");
//       Serial.print("R: ");
//       Serial.print(profile_isntance_R);
//       Serial.print("  G: ");
//       Serial.print(profile_instance_G);
//       Serial.print("  B: ");
//       Serial.println(profile_istance_B);
      
//   while(read) {

//     apds.readRedLight(red_light);
//     apds.readGreenLight(green_light);
//     apds.readBlueLight(blue_light);

//     float vect_dist = calculate_vector_dist(red_light, green_light, blue_light, 
//                                             profile_isntance_R, profile_instance_G, profile_istance_B);

//     current_millis = millis();
    
    

//     Serial.println("Actual RGB values...");
//       Serial.print("R: ");
//       Serial.print(red_light);
//       Serial.print("  G: ");
//       Serial.print(green_light);
//       Serial.print("  B: ");
//       Serial.println(blue_light);

//     delay(2000);
//     if(vect_dist >= 20) {
//       //update new color information
//       profile_isntance_R = red_light;
//       profile_instance_G = green_light;
//       profile_istance_B = blue_light;
//       prev_millis = current_millis - prev_millis;

//       //If the newly recorded colour profile already exists in the list, then you already recorded this value. 
//       //If you already recorded this value, you have captured all the color profiles. 
//       // if(!dupe_present_vect(R_profile, G_profile, B_profile, distance_threshold, 
//       //                       R_profile.get_size_dyn(), profile_isntance_R, profile_instance_G, 
//       //                       profile_istance_B)) 
//       // {

//         collect_queue_data(R_profile, profile_isntance_R);
//         collect_queue_data(G_profile, profile_instance_G);
//         collect_queue_data(B_profile, profile_istance_B);
//         collect_queue_data_float(time_profile, prev_millis);
//       //}

//       // else {
//       //   read = false;
//       // }

//     }

//     else if(current_millis - prev_millis >= TIME_OUT) {
//       //the sequence has timed out and you are not getting a new color profile. 
      
      
//       return false;
//     }
    
//   }
  
//   return true;

// }



