#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <SPI.h>
#include <queue.h>

// Pins
#define APDS9960_INT    2  // Needs to be an interrupt pin

// Constants
#define LIGHT_INT_HIGH  200 // High light level for interrupt
#define LIGHT_INT_LOW   1   // Low light level for interrupt
#define FLOAT_POINT 2
#define uINT16 1
#define COLD 1
#define WARM 2
/*
---------------GLOBAL VARIABLE DEFINITIONS---------------
*/

SparkFun_APDS9960 apds = SparkFun_APDS9960();
uint16_t ambient_light = 0;
uint16_t red_light = 0, green_light = 0, blue_light = 0;
volatile bool isr_flag = false;
uint16_t threshold = 0;
uint16_t integrationTime = 25;
uint16_t delayTime = 500;

//ideal case values of RGB for both warm and cold. 
//Will be the centre point of the acceptable margin 3D sphere. See extra documentation for mathematical explanation.  
const float ideal_delta_R_warm = 216.0;
const float ideal_delta_G_warm = 122.0;
const float ideal_delta_B_warm = 71.0;

const float ideal_delta_R_cold = 147.0;
const float ideal_delta_G_cold = 170.0;
const float ideal_delta_B_cold = 148.0;

const float max_dist_cold = 100.0;
const float max_dist_warm = 100.0;

//Queue initalization
const int arr_size = 4;

float calculate_avg_delta(circular_queue &q, const uint8_t size);

circular_queue delta_R_cold(arr_size, FLOAT_POINT);
circular_queue delta_G_cold(arr_size, FLOAT_POINT);
circular_queue delta_B_cold(arr_size, FLOAT_POINT);

circular_queue delta_R_warm(arr_size, FLOAT_POINT);
circular_queue delta_G_warm(arr_size, FLOAT_POINT);
circular_queue delta_B_warm(arr_size, FLOAT_POINT);

circular_queue R(arr_size, uINT16);
circular_queue G(arr_size, uINT16);
circular_queue B(arr_size, uINT16);

/*
---------------FUNCITON DECLARATIONS---------------
*/
//set-up function
void initialize();

//interrupt service routine
void interruptRoutine();
uint8_t interruptCounter = 0;

//Serial Monitor Interface 
void print_RGB();

//Queue helper functions
void print_queue(circular_queue &q);
void collect_queue_data(circular_queue &q, uint16_t &val);
void collect_queue_data_float(circular_queue &q, float val);

float calculate_avg(circular_queue &q, const uint8_t size);
float calculate_vector_dist(float x1, float y1, float z1, float x0, float y0, float z0);

//mode = 1 -> measuring COLD
//mode = 2 -> measuring WARM
uint8_t mode = COLD;

//data process
bool is_color_valid(float cold_v_dist, float warm_v_dist);

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
  } else {
    Serial.println(F("Something went wrong during APDS-9960 init!"));
  }

  // Set high and low interrupt thresholds
  if ( !apds.setLightIntLowThreshold(LIGHT_INT_LOW) ) {
    Serial.println(F("Error writing low threshold"));
  }
  if ( !apds.setLightIntHighThreshold(LIGHT_INT_HIGH) ) {
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
  if ( !apds.setAmbientLightIntEnable(1) ) {
    Serial.println(F("Error enabling interrupts"));
  }
  
  //change the time the diodes are exposed to light. Lower integration time means less sensitivity. 
  if( !apds.setADCIntegrationTime(integrationTime)) {
    Serial.println("Integration Time not set...");
  }
  
  if(delayTime <= 250) {
    Serial.println("Delay Time is too short. The program will not work as expected...");
  }

  // Wait for initialization and calibration to finish
  delay(500);

  delta_R_cold.print_elements_float();

}

void loop() {
  
  
  // Read the light levels (ambient, red, green, blue)
  //do not enter this loop when interrupt is handled. 
  if (  !apds.readAmbientLight(ambient_light) ||
        !apds.readRedLight(red_light) ||
        !apds.readGreenLight(green_light) ||
        !apds.readBlueLight(blue_light) ) 
  {
    Serial.println("Error reading light values");
  }

  // If interrupt occurs, process data...
  if (isr_flag) {
    
    Serial.println("    Interrupt!    ");
    
    float delta_R_non_ideal = red_light - calculate_avg(R, arr_size);
    float delta_G_non_ideal = green_light - calculate_avg(G, arr_size);
    float delta_B_non_ideal = blue_light - calculate_avg(B, arr_size);

    if(mode == COLD) {
      Serial.println(delta_R_non_ideal);
      collect_queue_data_float(delta_R_cold, delta_R_non_ideal);
      collect_queue_data_float(delta_G_cold, delta_G_non_ideal);
      collect_queue_data_float(delta_B_cold, delta_B_non_ideal);

      //mode = WARM;
    }
    else if(mode == WARM) {

      mode = COLD;
    }

    interruptCounter++;

    if(interruptCounter >= arr_size) {
      
      //disable apds
      delta_R_cold.print_elements_float();
      float delta_R_cold_non_ideal_avg = calculate_avg_delta(delta_R_cold, arr_size);
      float delta_G_cold_non_ideal_avg = calculate_avg_delta(delta_G_cold, arr_size);
      float delta_B_cold_non_ideal_avg = calculate_avg_delta(delta_B_cold, arr_size);

      //Serial.println(delta_R_cold_non_ideal_avg);
      // Serial.println(delta_R_cold_non_ideal_avg);
      // Serial.println(delta_G_cold_non_ideal_avg);
      // Serial.println(delta_B_cold_non_ideal_avg);

      // float delta_R_warm_non_ideal_avg = calculate_avg(delta_R_warm, arr_size);
      // float delta_G_warm_non_ideal_avg = calculate_avg(delta_G_warm, arr_size);
      // float delta_B_warm_non_ideal_avg = calculate_avg(delta_B_warm, arr_size);

      float distance_cold = calculate_vector_dist(
        delta_R_cold_non_ideal_avg, delta_G_cold_non_ideal_avg, delta_B_cold_non_ideal_avg,
        ideal_delta_R_cold, ideal_delta_G_cold, ideal_delta_B_cold);

      // float distance_warm = calculate_vector_dist(
      //   delta_R_warm_non_ideal_avg, delta_G_warm_non_ideal_avg, delta_B_warm_non_ideal_avg,
      //   ideal_delta_R_warm, ideal_delta_G_warm, ideal_delta_B_warm);

      if(is_color_valid(distance_cold, 100.0)) {
        Serial.println("TW: LPT Passed");
      }

      else {
        Serial.print("Not TW: LPT Failed    ");
        Serial.print("distance: ");
        Serial.println(distance_cold);
      }

      //re-enable sensor

      interruptCounter = 0;
    }

    // Reset flag and clear APDS-9960 interrupt (IMPORTANT!)
    isr_flag = false;
    if ( !apds.clearAmbientLightInt() ) {
      Serial.println("Error clearing interrupt");
    }
    
  }

  else if(!isr_flag) {
    collect_queue_data(R, red_light);
    collect_queue_data(G, green_light);
    collect_queue_data(B, blue_light);
  }

  

  print_RGB();
  
  
  

  delay(delayTime);
}


/*
---------------FUNCTION DEFINITIONS---------------
*/


void interruptRoutine() {
  isr_flag = true;
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

//something is wrong here...
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

bool is_color_valid(float cold_v_dist, float warm_v_dist) {
  return cold_v_dist <= max_dist_cold;
}
