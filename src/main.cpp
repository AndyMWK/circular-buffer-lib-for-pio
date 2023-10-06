#include <Wire.h>
#include <SPI.h>
#include <Adafruit_APDS9960.h>
#include <queue.h>

//global sensor variable
Adafruit_APDS9960 apds;

//global variables to configure the sensor behavior.         
const uint16_t integrationTime = 20;
const uint16_t delayTime = 2000;


//initalize RGBC values
uint16_t r = 0, g = 0, b = 0, c = 0;


//AILTL & AIHTH
const uint16_t high_clear = 900; //850 if the bounds are too strict
const uint16_t low_clear = 300;

const uint16_t high_cold = 1800;
const uint16_t low_cold = 9;

const int interruptPin = 3;

void interruptHandler();

//the possible amount of 
const int arr_size = 4;

circular_queue R(arr_size);
circular_queue G(arr_size);
circular_queue B(arr_size);
circular_queue C(arr_size);

//function declarations
void print_RGB_data();

void collect_queue_data(circular_queue &q, uint16_t &val);

void print_queue_data();

int8_t process_RGB(uint8_t mode);

//based on the existing RGB global circular queue objects
float calculate_vector_distance(float x, float y, float z);

float calculate_avg(circular_queue &q);

//ideal case values of RGB for both warm and cold. Will be the centre point of 
const float ideal_R_warm = 216;
const float ideal_G_warm = 122;
const float ideal_B_warm = 71;

const float ideal_R_cold = 147;
const float ideal_G_cold = 170;
const float ideal_B_cold = 148;


//set to arbitrary values
const float range_warm = 130;
const float range_cool = 130;

void setup() {
  //Begin serial monitor
  Serial.begin(9600);

  //establish I2C communication
  Wire.begin();

  //Do not proceed until the sensor is correctly initialized...
  while(!apds.begin()) {
      Serial.println("APDS 9960 could not be initalized");
  }

  //enable the colour sensing feature of the APDS-9960 sensor
  apds.enableColor(true);

  //change the time the infrared diodes are exposed to light to reduce saturation
  apds.setADCIntegrationTime(integrationTime);

  apds.setADCGain(APDS9960_AGAIN_1X);

  apds.enableColorInterrupt();

  //doesn't do what it is supposed to do but the sensor doesn't work without it...?
  apds.setIntLimits(low_clear, high_clear);

  attachInterrupt(digitalPinToInterrupt(interruptPin), interruptHandler, FALLING);
}

bool isr = false;

uint8_t mode = 1;

//int interrupt_counter = 0;
void loop() {

  //make sure that the sensor is ready to collect data...
  if(!apds.colorDataReady()) {
      Serial.println("sensor is not ready to read...");
      return;
  }

  apds.getColorData(&r, &g, &b, &c);
  // print_RGB_data();

  //calculate the change in clarity to detect whether we are looking at a birght light or not...
  uint16_t delta_c = c - C.get_rear();
  
  //print_queue_data();

  collect_queue_data(R, r);
  collect_queue_data(G, g);
  collect_queue_data(B, b);
  collect_queue_data(C, c);
  

  if(isr) {
  

    if(delta_c <= high_cold && delta_c >= low_cold ) {
      Serial.println("1");
      
    }
    
    //handle Interrupt Flag...
    isr = false;
    apds.clearInterrupt();
  }
  

  delay(delayTime);

}


//function definitions
void print_RGB_data() {
    Serial.print("RGB : ");
    Serial.print(r);
    Serial.print(", ");
    Serial.print(g);
    Serial.print(", ");
    Serial.print(b);
    Serial.print("  C = ");
    Serial.println(c);
}

void interruptHandler() {
  isr = true;
  //interrupt_counter++;
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

void print_queue_data() {
    C.print_elements();
}

float calculate_avg(circular_queue &q) {
  long sum = 0;
  for(uint8_t i = 0; i < arr_size; i++) {
    sum += q.get_index(i);
  }

  return static_cast<float>(sum)/static_cast<float>(arr_size);
}

float calculate_vector_distance(float x, float y, float z) {
  //calculate the distance based on the given origin point. 
  float R_average = calculate_avg(R);
  float G_average = calculate_avg(G);
  float B_average = calculate_avg(B);

  float distance = pow(R_average - x, 2) + pow(G_average - y, 2) + pow(B_average - z, 2);

  return sqrt(distance);

}



//when processing RGB, turn off APDS interrupt...
int8_t process_RGB(uint8_t mode) {

  //based on the mode, check the if the values fall within the indiphere...
  
  //return 0 if wrong

  //return -1 if error

<<<<<<< HEAD
  if(mode == 1) {

    //returns 1 when the vector distance is within the range
    //returns 0 when the vector distance is outside the range
    return calculate_vector_distance(ideal_R_warm, ideal_G_warm, ideal_B_warm) <= range_warm;
  }

  if(mode == 2) {

    //returns 1 when the vector distance is within the range
    //returns 0 when the vector distance is outside the range
    return calculate_vector_distance(ideal_R_cold, ideal_G_cold, ideal_B_cold) <= range_cool;
  }

  else {

    //returns -1 when there is something wrong. mode can't be anything other than 1 or 2
    return -1;
  }
}
=======
}
>>>>>>> 2a687e56ec44382062c357f4e856f822abb6b062
