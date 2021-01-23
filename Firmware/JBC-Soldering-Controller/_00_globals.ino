/* Copyright (c) 2017 timothyjager
   JBC-Soldering-Controller
   MIT License. See LICENSE file for details.
*/

//----------------Pin Mapping-------------------------
//Arduino Pro-Micro Pinout
//https://cdn.sparkfun.com/assets/9/c/3/c/4/523a1765757b7f5c6e8b4567.png
const int ENC_A            = 0;//= 1;
const int ENC_B            = 1;//= 6;
const int I2C_SDA          = 2;
const int I2C_SCL          = 3;
const int CS               = 4;
const int LOWMOS_0         = 6;
const int ENC_BUTTON         = 7; 
const int WS2812_DATA      = 5;
const int CRADLE_SENSOR    = 8;
const int LPINA            = 9;  //Heater PWM
const int VIN_SENSE        = A3;
const int CURRENT_SENSE0   = A2;//= 20;
const int CURRENT_SENSE1   = A1;
const int DBG_OUTPUT       = A0;
const int SPI_SCLK         = 15;
const int SPI_MISO         = 14;
const int SPI_MOSI         = 16;
const int LPINB            = 10;  //Timer 1 Debug Pin
//-----------------------------------------------------

bool low_mosfet_state = false;
bool mosfet_stateA = true;
bool mosfet_stateB = true;

//----------------Settings-----------------------------
// You should change these settings depending on your situation.
const bool disable_simultaneous_output = false;// If true, this will not allow both outputs to be on at the same time.

//-----------------------------------------------------

//----------------Function Prototypes------------------
void PulsePin(int pin);
void updateLEDStatus(void);
void ProcessSerialComm(void);
void updateDisplay(bool update_now);
//-----------------------------------------------------

const uint16_t PixelCount = 4; // this example assumes 4 pixels, making it smaller will cause a failure
#define colorSaturation 128
#define coldTemp 80
bool cradle_present = false;
bool iron_active =  false;
int upper_set_limit = 450;
int tip_check_cnt = 1;
float vSense0 = 0.0;
float vSense1 = 0.0;
float vInDev = 0.0;

//----------------Structure Definitions----------------

//System Parameters Data Structure
typedef struct {
  byte  pid_mode;          //PID mode - Automatic=1, Manual=0
  //byte  simulate_input;     //this allows us to override the actual input (temperature reading) using the tuning app
  int16_t idle_temp_c;
  int16_t output_override;
  float setpoint;
  //float kP;
  //float kI;
  //float kD;
  float simulated_input;
  float tipResistance0;
  float tipResistance1;
  int iron;
} system_parameters_struct;

//Status Variables Struct - hold global status values
typedef struct {
  //byte gpio_port_b;                //Port b of GPIO
  //byte gpio_port_c;                //Port C of GPIO
  //byte gpio_port_d;                //Port d of GPIO
  //byte gpio_port_e;                //Port e of GPIO
  int16_t encoder_pos;             //Enocder Position
  int16_t adapter_voltage_mv;      //Input power adapter voltage in millivolts
  int16_t adcval;                  //ADC value read by ADS1220
  int16_t tempCJ;                  //internal temp of ADS1220
  int16_t adcCJ;                   //internal adc counts of ADS1220
  float voltCJ;                    //internal voltage of ADS1120
  int16_t current_sense_mv;        //current sense in milliamps
  double pid_setpoint;             //setpoint of the PID loop
  double tip_temperature_c;        //input value of the PID loop
  double tip_temperature_c2;
  double pid_output;              //computed output value of the PID loop
  double pid_output2;
} status_struct;



//----------------Standard Global Variables----------------
status_struct status;
system_parameters_struct params;

//Hard-coded calibration points.  TODO: make these not hard-coded
#define NUM_CAL_POINTS 4
uint16_t adc_reading [NUM_CAL_POINTS] = {283, 584, 919, 1098};
uint16_t deg_c [NUM_CAL_POINTS] = {105, 200, 300, 345};

#define kP 30
#define kI 1
#define kD 0

#define iron_none 0
#define iron_T245 1
#define iron_T210 2

//----------------Globals Objects----------------------
Adafruit_SSD1306 display(-1);     //TODO: look into this reset pin. The LCD i'm using does not have a reset pin, just PWR,GND,SDA,SCL
Encoder knob(ENC_A, ENC_B);               //Setup the encoder object
NeoPixelBus<NeoRgbwFeature, Neo800KbpsMethod> pixels(PixelCount, WS2812_DATA);
PID myPID(&status.tip_temperature_c, &status.pid_output, &status.pid_setpoint, kP, kI, kD, P_ON_E, DIRECT); //TODO: map this properly to NVOL data storage
PID myPID2(&status.tip_temperature_c2, &status.pid_output2, &status.pid_setpoint, kP, kI, kD, P_ON_E, DIRECT); //TODO: map this properly to NVOL data storage
