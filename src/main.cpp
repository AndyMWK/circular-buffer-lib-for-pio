#include <Wire.h>
#include "SparkFun_APDS9960.h"
#include <SPI.h>

//dependencies to help queue functionality
#include "queue.h"
#include "queue_helper.h"

//includes RGB Color Space Functions
#include "RGB_vector.h"

//Pins
#define APDS9960_INT    2  // Needs to be an interrupt pin

// Constants

/*
DELAY_TIME and SCAN_TIME will determine the speed and accuracy of the program. 

START OFF BY ADJUSTING THESE VALUES ONLY
*/
#define LIGHT_INT_HIGH  100 // High light level for interrupt
#define LIGHT_INT_LOW   0   // Low light level for interrupt
#define FLOAT_POINT 2
#define uINT16 1
#define DELAY_TIME 65
#define SCAN_TIME 325
#define MAX_NUM_COLOR 2
#define SAMPLE_SIZE 3
#define GND_VAL_THRESH 170
#define DIFFERENT_COLORS_THRESH 23.5
#define STD_RGB_VECT 30
#define DOMINANT_COLOR_PERCENT_THRESH 0.45



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

//circular queue/buffer initialization processes

circular_queue<uint16_t> R_background(background_arr_size);
circular_queue<uint16_t> G_background(background_arr_size);
circular_queue<uint16_t> B_background(background_arr_size);

circular_queue<float> collected_profile_R(arr_size);
circular_queue<float> collected_profile_G(arr_size);
circular_queue<float> collected_profile_B(arr_size);

circular_queue<float> pulse_avg_R(SAMPLE_SIZE);
circular_queue<float> pulse_avg_G(SAMPLE_SIZE);
circular_queue<float> pulse_avg_B(SAMPLE_SIZE);


/*
---------------FUNCITON DECLARATIONS---------------
*/


//interrupt service routine
volatile bool isr_flag = false;
void interruptRoutine();
void handle_interrupt();

//Sensor initialization function
void enable_apds();

//Read from sensor
void read_RGB();

//Serial Monitor Interface 
void print_RGB();
float R_background_avg = 0.0, G_background_avg = 0.0, B_background_avg = 0.0;

//ALL SIGNAL PROCESSING AND DECISION MAKING FUNCTIONS
float calculate_std(float* arr, int size);
bool RGB_consistent(float max_std);

bool process_pulse_avg();
int differentiate_colors_preprocess(float deviation);
int find_dominant_num_of_color(int num_color_list[SAMPLE_SIZE], int count_color);

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
  
  //enable all sensor functions
  enable_apds();
}

/*
These are global variables that change their state constantly. 

Depending on their state, the program will execute a different task. 

These tasks include: 
        - Data collection
        - Data processing
        - Final decision making

The program operates on the princicple of STATE MACHINES
*/

bool skipped_start = false;
bool process_data = false;
int data_collected = 0;
int pulse_avg_processed = 0;
int num_colors = 0;
bool re_checking = false;
int re_checking_count = 0;

int num_colors_list [SAMPLE_SIZE] = {};

void loop() {

  //Will first read values from the sensor
  read_RGB();

  /*
  If the sensor detects a bright light, isr_flag will be set to true
  else it will think that the collected sensor values are background. 
  */
  if(!isr_flag) {
    queue_helper::collect_queue_data(R_background, red_light);
    queue_helper::collect_queue_data(G_background, green_light);
    queue_helper::collect_queue_data(B_background, blue_light);


    /*
    The program will use the average values of the background.
    It will collect a few or several background values and take the average. 
    The average of these background data will be treated as the background values. 
    */

    R_background_avg = queue_helper::calculate_avg(R_background, background_arr_size);
    G_background_avg = queue_helper::calculate_avg(G_background, background_arr_size);
    B_background_avg = queue_helper::calculate_avg(B_background, background_arr_size);
  }

  
  //When the program enters this if statement, it means that it has detected a bright light
  else if(isr_flag) {

    //interrupt is immidiately handled
    handle_interrupt();
    
    //it will skip the start of the lighting pattern because it is different from the rest
    if(!skipped_start) {
        skipped_start = true;
        delay(1100);
        return;
    }

    //DATA COLLECTION STATE
    if(data_collected < arr_size) {
      float red_light_float = red_light*1.0;
      float green_light_float = green_light*1.0;
      float blue_light_float = blue_light*1.0;
      queue_helper::collect_queue_data(collected_profile_R, red_light_float);
      queue_helper::collect_queue_data(collected_profile_G, green_light_float);
      queue_helper::collect_queue_data(collected_profile_B, blue_light_float);

      data_collected++;

    } 
    
    //SIGNAL PREPROCESSING STATE
    else if(data_collected == arr_size) {

      //----------UNCOMMENT THE PRINT STATEMENTS BELOW FOR DEBUGGING------------
      // Serial.println("------RGB Array------");

      // collected_profile_R.print_elements_float();
      // collected_profile_G.print_elements_float();
      // collected_profile_B.print_elements_float();
      
      //additional function for preprocessing the amount of different colors in the array
      num_colors = differentiate_colors_preprocess(DIFFERENT_COLORS_THRESH);

      //----------UNCOMMENT THE PRINT STATEMENTS BELOW FOR DEBUGGING------------
      // if(num_colors == 1) {
      //   // Serial.println("Single Channel Lum");
      // } if(num_colors == 2) {
      //   // Serial.println("TW");
      // } else if(num_colors > 2){
      //   // Serial.print("detected colors are: ");
      //   // Serial.println(num_colors);
      // }

      num_colors_list[pulse_avg_processed] = num_colors;

      if(process_pulse_avg()) {
        pulse_avg_processed++;
        
      }

      //TRANSITION TO DATA COLLECTION STATE
      data_collected = 0;
      collected_profile_R.reset();
      collected_profile_G.reset();
      collected_profile_B.reset();
     
    }
    
    //TRANSITION TO FINAL PROCESSING STATE
    if(pulse_avg_processed == SAMPLE_SIZE) {

      //----------UNCOMMENT THE PRINT STATEMENTS BELOW FOR DEBUGGING------------

      // Serial.println("---Pulse Average---");
      // pulse_avg_R.print_elements_float();
      // pulse_avg_G.print_elements_float();
      // pulse_avg_B.print_elements_float();
      // Serial.print("[ ");
      // for(int i = 0; i < pulse_avg_processed; i++) {
      //   Serial.print(num_colors_list[i]);
      //   Serial.print(" ");
      // }
      // Serial.println(" ]");
      process_data = true;
    }
    
    //FINAL PROCESSING STATE
    if(process_data) {

      
      if(RGB_consistent(STD_RGB_VECT)) { 
        Serial.println("LPT PASSED");
      }
      else {
        Serial.println("LPT FAILED");
      }

      int num_colors_dominant = find_dominant_num_of_color(num_colors_list, arr_size);
      
      if(num_colors_dominant == -1 && !re_checking) {
        Serial.println("Test Failed, re-checking...");
        re_checking = true;
      }

      //REDUNDANCY MECHANISM
      else if(re_checking) {

        re_checking_count++;

        if(num_colors_dominant == -1) {
          re_checking = false;
          re_checking_count = 0;
          Serial.println("Re-checking complete. Bad");
        }
        else if(re_checking_count >= 1) {
          re_checking = false;
          re_checking_count = 0;
          Serial.println("Re-checking complete. Good");
        }
      }
      
      Serial.print("Number of Colors are: ");
      Serial.println(num_colors_dominant);

      //TRANSITION TO DATA COLLECTION STATE
      process_data = false;
      pulse_avg_processed = 0;
      data_collected  = 0;

      pulse_avg_R.reset();
      pulse_avg_G.reset();
      pulse_avg_B.reset();

      num_colors = 0;

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

//Prints all current background RGB averages. Used for debugging
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

//standard deviation calculation. Used in Final Processing State
float calculate_std(float* arr, int size) {

  //----------UNCOMMENT THE PRINT STATEMENTS BELOW FOR DEBUGGING------------

  float sum = 0.0;
  // Serial.println("----RGB Distance----");
  for(int i = 0; i < size; i++) {
    // Serial.print("Avg dist: ");
    // Serial.println(arr[i]);
    sum += arr[i];
  }
  float avg = sum/(size*1.0);

  float top = 0.0;
  for(int i = 0; i < size; i++) {
    top += pow(arr[i] - avg, 2);
  }

  return sqrt(top/(size - 1)*1.0);
}

//higher level algorithm to detect the consistency of collected RGB sensor values
//Used in Final Processing State
bool RGB_consistent(float max_std) {

  float* arr = new float[SAMPLE_SIZE];

  for(int i = 0; i < SAMPLE_SIZE; i++) {
    arr[i] = RGB_vector::calculate_vector_dist(pulse_avg_R.get_index(i), pulse_avg_G.get_index(i), 
                                                pulse_avg_B.get_index(i), origin_x, origin_y, origin_z);
  }

  float std = calculate_std(arr, SAMPLE_SIZE);

  delete[] arr;

  if(std >= max_std) {
    return false;
  }

  //----------UNCOMMENT THE PRINT STATEMENTS BELOW FOR DEBUGGING------------

  // Serial.print("STD: ");
  // Serial.println(std);

  
  return true;
}

/*
Intermediate preprocessing function. 
This function filters out background RGB readings and takes the average value
of all non-background RGB values. 

In other words, this function looks at the "average" color of the collected RGB data
*/
bool process_pulse_avg() {
      
  float total_sum_R = 0;
  float total_sum_G = 0;
  float total_sum_B = 0;

  int how_many_collected = 0;

  for(int i = 0; i < arr_size; i++) {
        
          if(queue_helper::is_within_percent_treshold(
            collected_profile_R.get_index(i), R_background_avg, GND_VAL_THRESH)) {
          
            continue;
          }
          total_sum_R += collected_profile_R.get_index(i);
          total_sum_G += collected_profile_G.get_index(i);
          total_sum_B += collected_profile_B.get_index(i);

          how_many_collected++;
        
    }

    //----------UNCOMMENT THE PRINT STATEMENTS BELOW FOR DEBUGGING------------

    // Serial.println("----Collected Sum----");
    // Serial.print("R: ");
    // Serial.print(total_sum_R);
    // Serial.print("  G: ");
    // Serial.print(total_sum_G); 
    // Serial.print("  B: ");
    // Serial.println(total_sum_B);

    // Serial.println("----# of collected data points----");
    // Serial.print("#: ");
    // Serial.println(how_many_collected);

    if(how_many_collected == 0) {
      return false;
    }

    queue_helper::collect_queue_data(pulse_avg_R, total_sum_R/how_many_collected);
    queue_helper::collect_queue_data(pulse_avg_G, total_sum_G/how_many_collected);
    queue_helper::collect_queue_data(pulse_avg_B, total_sum_B/how_many_collected);

    return true;
}


/*
color detecting algorithm used in the preprocessing state. 
This algorithm gives an integer representing the number of colors it has detected. 

In terms of accuracy and speed performance of the program, THIS IS THE BOTTLENECK
*/
int differentiate_colors_preprocess(float deviation) {

  int count = 0;
  int* arr = new int[MAX_NUM_COLOR];

  for(int i = 0; i < MAX_NUM_COLOR; i++) {
    arr[i] = R_background_avg;
  }
  
  int index = 0;
  for(int i = 0; i < arr_size; i++) {
    int value_frequency = 0;
    if(queue_helper::is_within_percent_treshold(collected_profile_R.get_index(i), R_background_avg, GND_VAL_THRESH)) {
          
            continue;
    }

    for(int j = 0; j < MAX_NUM_COLOR; j++) {
      if(queue_helper::is_within_percent_treshold(collected_profile_R.get_index(i), arr[j], deviation)) {
        value_frequency++;
      }
    }

    if(value_frequency == 0) {
      arr[index] = collected_profile_R.get_index(i);
      count++;
      index = (index + 1) % MAX_NUM_COLOR;
      
    }

  }

  delete[] arr;
  return count;
}


//MAGIC NUMBER PRESENT
int find_dominant_num_of_color(int num_color_list[SAMPLE_SIZE], int count_color) {
  int counts[count_color] = {0};
  for(int i = 0; i < SAMPLE_SIZE; i++) {
    counts[num_color_list[i]]++;
  }

  //----------UNCOMMENT THE PRINT STATEMENTS BELOW FOR DEBUGGING------------

  // Serial.println("Counts Array: ");
  // for(int i = 0; i < count_color; i++) {
  //   Serial.print(counts[i]);
  //   Serial.print("  ");
  // }

  int max_count = counts[0];
  int max_count_index = 0;
  for(int i = 0; i < count_color; i++) {
    if(counts[i] > max_count) {
      max_count_index = i;
      max_count = counts[i];
    }
  }

  int count_of_dominant_color = 0;
  for(int i = 0; i < SAMPLE_SIZE; i++) {
    if(num_color_list[i] == max_count_index) {
      count_of_dominant_color++;
    }
  }

  //MAGIC NUMBER -> 0.45
  if(max_count_index > 2 || max_count_index < 1) {
    return -1;
  }
  
  if((count_of_dominant_color*1.0 / SAMPLE_SIZE) >= DOMINANT_COLOR_PERCENT_THRESH) {
    return max_count_index;
  } else {
    return -1;
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
  delay(DELAY_TIME*10);
}
