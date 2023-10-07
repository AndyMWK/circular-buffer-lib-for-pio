#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <SPI.h>
#include <queue.h>

// Pins
#define APDS9960_INT    2  // Needs to be an interrupt pin

// Constants
#define LIGHT_INT_HIGH  200 // High light level for interrupt
#define LIGHT_INT_LOW   5   // Low light level for interrupt

/*
---------------GLOBAL VARIABLE DEFINITIONS---------------
*/

SparkFun_APDS9960 apds = SparkFun_APDS9960();
uint16_t ambient_light = 0;
uint16_t red_light = 0, green_light = 0, blue_light = 0;
volatile bool isr_flag = false;
uint16_t threshold = 0;
uint16_t integrationTime = 25;
uint16_t delayTime = 1000;

//ideal case values of RGB for both warm and cold. 
//Will be the centre point of the acceptable margin 3D sphere. See extra documentation for mathematical explanation.  
const float ideal_R_warm = 216;
const float ideal_G_warm = 122;
const float ideal_B_warm = 71;

const float ideal_R_cold = 147;
const float ideal_G_cold = 170;
const float ideal_B_cold = 148;

//Queue initalization
const int arr_size = 4;
circular_queue delta_R(arr_size);
circular_queue delta_G(arr_size);
circular_queue delta_B(arr_size);

/*
---------------FUNCITON DECLARATIONS---------------
*/

//interrupt service routine
void interruptRoutine();

//Serial Monitor Interface 
void print_RGB();

//Queue helper functions
void print_queue(circular_queue &q);
void collect_queue_data(circular_queue &q, uint16_t &val);



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
  
  // Wait for initialization and calibration to finish
  delay(500);
}

void loop() {
  
  // If interrupt occurs, process data...
  
  if ( isr_flag == 1 ) {
    
      Serial.println("    Interrupt!    ");
    
    // Reset flag and clear APDS-9960 interrupt (IMPORTANT!)
    isr_flag = false;
    if ( !apds.clearAmbientLightInt() ) {
      Serial.println("Error clearing interrupt");
    }
    
  }
   // Read the light levels (ambient, red, green, blue) and print
  if (  !apds.readAmbientLight(ambient_light) ||
        !apds.readRedLight(red_light) ||
        !apds.readGreenLight(green_light) ||
        !apds.readBlueLight(blue_light) ) 
  {
    Serial.println("Error reading light values");
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

void print_queue_data(circular_queue &q) {
    q.print_elements();
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