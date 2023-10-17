#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <SPI.h>
#include <queue.h>


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

/*
---------------GLOBAL VARIABLE DEFINITIONS---------------
*/

SparkFun_APDS9960 apds = SparkFun_APDS9960();
uint16_t ambient_light = 0;
uint16_t red_light = 0, green_light = 0, blue_light = 0;
volatile bool isr_flag = false;
uint16_t threshold = 0;
uint16_t integrationTime = 25;


//Queue initalization
const int arr_size = 4;

float calculate_avg_delta(circular_queue &q, const uint8_t size);

circular_queue delta_R(arr_size, FLOAT_POINT, DYNAMIC_TRUE);
circular_queue delta_G(arr_size, FLOAT_POINT, DYNAMIC_TRUE);
circular_queue delta_B(arr_size, FLOAT_POINT, DYNAMIC_TRUE);

circular_queue R_profile(arr_size, uINT16, DYNAMIC_TRUE);
circular_queue G_profile(arr_size, uINT16, DYNAMIC_TRUE);
circular_queue B_profile(arr_size, uINT16, DYNAMIC_TRUE);

circular_queue R_background(arr_size, uINT16, DYNAMIC_FALSE);
circular_queue G_background(arr_size, uINT16, DYNAMIC_FALSE);
circular_queue B_background(arr_size, uINT16, DYNAMIC_FALSE);

circular_queue time_profile(arr_size, FLOAT_POINT, DYNAMIC_TRUE);
circular_queue distance_data(arr_size, FLOAT_POINT, DYNAMIC_TRUE);

/*
---------------FUNCITON DECLARATIONS---------------
*/

void initialize();

void collect_queue_data(circular_queue &q, uint16_t &val);
void collect_queue_data_float(circular_queue &q, float val);

float calculate_avg(circular_queue &q, const uint8_t size);
float calculate_vector_dist(float x1, float y1, float z1, float x0, float y0, float z0);

//data process
bool is_color_valid(float cold_v_dist, float warm_v_dist);
void print_RGB_distance(float distance_warm, float distance_cold);

bool initial_scanning();

void read_RGB();

//interrupt service routine
void interruptRoutine();

//Serial Monitor Interface 
void print_RGB();

void handle_interrupt();

bool initializeation_complete = false;

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

  // If interrupt occurs, process data...
  if (isr_flag) {
    apds.disableLightSensor();   

    if(R_background.is_empty()) {
      handle_interrupt();
      return; 
    }

    Serial.println("    Interrupt!    ");
    
    if(!initializeation_complete) {

      if(!initial_scanning()) {
        Serial.println("Scanning Procedure timed out...");
      }

      initializeation_complete = true;

      return;
    } else {
      float R_back_avg = calculate_avg(R_background, arr_size);
      float G_back_avg = calculate_avg(G_background, arr_size);
      float B_back_avg = calculate_avg(B_background, arr_size);

      float delta_R_instance = red_light - R_back_avg;
      float delta_G_instance = green_light - G_back_avg;
      float delta_B_instance = blue_light - B_back_avg;

      collect_queue_data_float(delta_R, delta_R_instance);
      collect_queue_data_float(delta_G, delta_G_instance);
      collect_queue_data_float(delta_B, delta_B_instance);
    }

  
    if(delta_R.get_size_dyn() >= R_profile.get_size_dyn() &&
        delta_G.get_size_dyn() >= G_profile.get_size_dyn() &&
        delta_B.get_size_dyn() >= B_profile.get_size_dyn()) {
      
      for(int i = 0; i < R_profile.get_size_dyn(); i++) {

        float RGB_dist = calculate_vector_dist(delta_R.get_index_float(i), delta_G.get_index_float(i), 
                                                delta_B.get_index_float(i),R_profile.get_index(i), G_profile.get_index(i), 
                                                B_profile.get_index(i));

        if(RGB_dist >= MAX_VECT_DIST) {
          Serial.print("Max RGB Distance reached or higher");
          Serial.print("    RGB Distance: ");
          Serial.println(RGB_dist);

          collect_queue_data_float(distance_data, RGB_dist);
        }
      }
    }

    // Reset flag and clear APDS-9960 interrupt (IMPORTANT!)
    handle_interrupt();
    //re-enable sensor
    initialize();
  }

  else if(!isr_flag) {
    collect_queue_data(R_background, red_light);
    collect_queue_data(G_background, green_light);
    collect_queue_data(B_background, blue_light);
  }

  print_RGB();

  delay(DELAY_TIME);
}


/*
---------------FUNCTION DEFINITIONS---------------
*/


void interruptRoutine() {
  isr_flag = true;
}

void initialize() {
  if ( apds.init() ) {
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
  } else {
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


void collect_queue_data(circular_queue &q, uint16_t &val) {
    q.enqueue(val);

    if(!q.is_full()) {

        return;
    }

    if(!q.is_empty()) {
        q.dequeue();
    }
}

void collect_queue_data_float(circular_queue &q, float val) {
    q.enqueue_float(val);

    if(!q.is_full()) {

        return;
    }

    if(!q.is_empty()) {
        q.dequeue_float();
    }
}

float calculate_avg(circular_queue &q, const uint8_t size) {
  float sum = 0;
  for(int i = 0; i < size; i++) {
    sum += q.get_index(i);
  }

  return sum/size;
}

float calculate_avg_delta(circular_queue &q, const uint8_t size) {
  float sum = 0;
  for(int i = 0; i < size; i++) {
    sum += q.get_index_float(i);
  }

  return sum/size;
}

float calculate_vector_dist(float x1, float y1, float z1, float x0, float y0, float z0) {

  float dist = pow(x1-x0, 2) + pow(y1-y0, 2) + pow(z1-z0, 2);

  return sqrt(dist);
}

void print_RGB_distance(float distance_warm, float disatnce_cold) {
  Serial.println();
  Serial.print("---warm distance: ");
  Serial.print(distance_warm);
  Serial.print("      ---cold distance: ");
  Serial.println(disatnce_cold);
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


uint16_t distance_threshold = 50;

bool dupe_present_vect(circular_queue &qx, circular_queue &qy, circular_queue &qz, 
                        float threshold, int array_size, uint16_t x, uint16_t y, uint16_t z) 
{
  for(int i = 0; i < array_size; i++) {
    float vect_dist = calculate_vector_dist(qx.get_index(i), qy.get_index(i), qz.get_index(i), x, y, z);

    if(vect_dist <= threshold) {
      return true;
    }
  }

  return false;
}

//need to account for the background
bool initial_scanning() {
  
  bool read = true;
  

  uint16_t profile_isntance_R = 0, profile_instance_G = 0, profile_istance_B = 0;

  // float R_back_avg = calculate_avg(R_background, arr_size);
  // float G_back_avg = calculate_avg(G_background, arr_size);
  // float B_back_avg = calculate_avg(B_background, arr_size);

  apds.readRedLight(profile_isntance_R);
  apds.readGreenLight(profile_instance_G);
  apds.readBlueLight(profile_istance_B);

  collect_queue_data(R_profile, profile_isntance_R);
  collect_queue_data(G_profile, profile_instance_G);
  collect_queue_data(B_profile, profile_istance_B);

  long current_millis = millis();

  long prev_millis = millis();

  Serial.println("Profile Instance Values...");
      Serial.print("R: ");
      Serial.print(profile_isntance_R);
      Serial.print("  G: ");
      Serial.print(profile_instance_G);
      Serial.print("  B: ");
      Serial.println(profile_istance_B);
      
  while(read) {

    apds.readRedLight(red_light);
    apds.readGreenLight(green_light);
    apds.readBlueLight(blue_light);

    float vect_dist = calculate_vector_dist(red_light, green_light, blue_light, 
                                            profile_isntance_R, profile_instance_G, profile_istance_B);

    current_millis = millis();
    
    

    Serial.println("Actual RGB values...");
      Serial.print("R: ");
      Serial.print(red_light);
      Serial.print("  G: ");
      Serial.print(green_light);
      Serial.print("  B: ");
      Serial.println(blue_light);

    delay(2000);
    if(vect_dist >= 20) {
      //update new color information
      profile_isntance_R = red_light;
      profile_instance_G = green_light;
      profile_istance_B = blue_light;
      prev_millis = current_millis - prev_millis;

      //If the newly recorded colour profile already exists in the list, then you already recorded this value. 
      //If you already recorded this value, you have captured all the color profiles. 
      if(!dupe_present_vect(R_profile, G_profile, B_profile, distance_threshold, 
                            R_profile.get_size_dyn(), profile_isntance_R, profile_instance_G, 
                            profile_istance_B)) 
      {

        collect_queue_data(R_profile, profile_isntance_R);
        collect_queue_data(G_profile, profile_instance_G);
        collect_queue_data(B_profile, profile_istance_B);
        collect_queue_data_float(time_profile, prev_millis);
      }

      else {
        read = false;
      }

    }

    else if(current_millis - prev_millis >= TIME_OUT) {
      //the sequence has timed out and you are not getting a new color profile. 
      
      
      return false;
    }
    
  }
  
  return true;

}



