/* Copyright (c) 2017 timothyjager
   JBC-Soldering-Controller
   MIT License. See LICENSE file for details.
*/

//----------------Setup-------------------------
void setup(void)
{
  //IO Pin Configuration
  fastPinMode(CRADLE_SENSOR, INPUT_PULLUP);    //setup cradle detect
  fastPinMode(ENC_BUTTON, INPUT_PULLUP);       //setup encoder button
  fastPinMode(LOWMOS_0, INPUT);               //low-sided mosfet 0
  //fastPinMode(LOWMOS_1, OUTPUT);               //low-sided mosfet 1 
  fastPinMode(CS, OUTPUT);                     //SPI chip select pin
  fastPinMode(DBG_OUTPUT,OUTPUT);

  //Serial Port
  Serial.begin(115200);

  //Neopixel
  pixels.Begin();                                  // This initializes the NeoPixel library.
  pixels.SetPixelColor(0, RgbwColor(0, 0, 10)); // Moderately bright green color.
  pixels.SetPixelColor(0, RgbwColor(1, 0, 10)); // Moderately bright green color.
  pixels.SetPixelColor(0, RgbwColor(2, 0, 10)); // Moderately bright green color.
  pixels.SetPixelColor(0, RgbwColor(3, 0, 10)); // Moderately bright green color.
  pixels.Show();

  //Setup the OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32). Module set to generate the OLED driver voltage internally
  display.setRotation(0);
  display.clearDisplay();                     // Clear the buffer.

  //Init for the ADS1220 ADC
  delayMicroseconds(50);
  SPI.begin();
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE1));
  fastDigitalWrite(CS, LOW);
  delayMicroseconds(1);
  SPI.transfer(0x06); // resetting device
  delayMicroseconds(100);
  //setting up configuration registers 0 to 3
  SPI.transfer(0x43); // bits 4:7 wreg command, bits 2:3 write to register 0, bits 0:1 data length is 4 bytes
  SPI.transfer(0x0E); // bits 4:7 set mux to ch0, bits 1:3 set gain to 128, bit 0 PGA bypass
  SPI.transfer(0x06); // bits 5:7 data rate 20sps, bits 3:4 set operating mode to normal, bit 2 sets continuous(1)/single-shot(0) mode, bit 1 disables(0)/enables(1) temperature sensor mode, bit 0 disables(0)/enables(1) burn-out current source
  SPI.transfer(0x10); //
  SPI.transfer(0x02);
  SPI.transfer(0x23); // reading back configuration registers
  long data = SPI.transfer(0x00);
  for (int i=0; i<3; i++)
    {data = (data<<8)|SPI.transfer(0x00);}
  //while(!Serial){;}
  Serial.println(data);
  SPI.transfer(0x08);
  delayMicroseconds(1);
  // measuring cold junction temperature
  while (digitalRead(SPI_MISO)==HIGH)
  {;};
  status.adcCJ = SPI.transfer(0x00);
  status.adcCJ = (status.adcCJ<<8)|SPI.transfer(0x00);
  Serial.println(status.adcCJ);
  status.tempCJ = (status.adcCJ>>2)*0.03125;
  Serial.println(status.tempCJ);
  status.voltCJ = CJtemp2volt(status.tempCJ);
  delayMicroseconds(1);
  
  SPI.transfer(0x06); // resetting device
  delayMicroseconds(100);
  SPI.transfer(0x43);
  SPI.transfer(0x0E);
  SPI.transfer(0xA0); // disabling temperature sensor mode
  SPI.transfer(0x10);
  SPI.transfer(0x02);
  SPI.transfer(0x08);
  delayMicroseconds(1);
  fastDigitalWrite(CS, HIGH);
   
  //Interrupts
  /*
     The Timer1 interrupts fire at the rising and falling edges of the PWM pulse.
     A is used to control our main PWM power to the heater
     B is used for sampling the ADC during the offtime of A
     We only care about B for ADC sampling, but we need to know if the interrupt is the rising or falling edge.
     We want to sample the ADC right after the falling edge of B, and the internal temp after the rising edge of B
     Since the falling edge of B always occurs after either edge of A (since B always has a higher duty cycle), we can enable the B interrupt from A
     and then just keep track of which edge we are. A bit is toggled each time the interrupt fires to remember if it's the rising or falling edge.
     This assumes we will never miss an interrupt.
     WE should probably add a handler in case the interrupt gets messed up and we get out of sync.
  */
  //setup PWM and interrupts. These use the 16 bit timer1
#define PWM_PERIOD_US 100000                           //20000us = 20ms = 50Hz PWM frequency. We have to go slow enough to allow time for sampling the ADC during the PWM off time
#define PWM_PERIOD_MS (PWM_PERIOD_US/1000)             //20000/1000 = 20ms
#define PWM_MAX_DUTY 1023                              //the timer 1 libray scales the PWM duty cycle from 0 to 1023 where 0=0% and 1023=100%
#define ADC_SAMPLE_WINDOW_US 5000                      //1200us = 1.2ms //we need 1.2 ms to sample the ADC. assuming 860 SPS setting
#define ADC_SAMPLE_WINDOW_PWM_DUTY (unsigned int)(((PWM_PERIOD_US-ADC_SAMPLE_WINDOW_US)*PWM_MAX_DUTY)/PWM_PERIOD_US)-50                 //(((PWM_PERIOD_US-ADC_SAMPLE_WINDOW_US)*PWM_MAX_DUTY)/PWM_PERIOD_US)  // we set our PWM duty to as close to 100% as possible while still leaving enough time for the ADC sample.
#define MAX_HEATER_PWM_DUTY ADC_SAMPLE_WINDOW_PWM_DUTY //our maximum allowable heater PWM duty is equal to the sampling window PWM duty.  

  Timer1.initialize(PWM_PERIOD_US);                    //Set timer1 to our main PWM period
  Timer1.pwm(LPINB, ADC_SAMPLE_WINDOW_PWM_DUTY+102);       //Set an interupt to define our sample window
  Timer1.pwm(LPINA, 60);                               //Set a default PWM value for our output
  Timer1.pwm(11, ADC_SAMPLE_WINDOW_PWM_DUTY+102);
  delay(PWM_PERIOD_MS);                                //make sure both PWM's have run at least one full period before enabling interrupts

  //low_mosfet_state = true;
  //fastDigitalWrite(LOWMOS_0, low_mosfet_state);
  //fastDigitalWrite(LOWMOS_1, low_mosfet_state);
  
  //MsTimer2::set(PWM_PERIOD_MS-1, flash);               // -1.5 from -2+0.5 delay in interrupt for adc reading
  TIFR1 |= _BV(OCF1A);                                 //clear the A interrupt flag, so it doesn't fire right away, when we enable the A interrupt
  TIFR1 |= _BV(OCF1B);
  TIFR1 |= _BV(OCF1C);
  //TIMSK1 = _BV(OCIE1A);                                //enable comparator A interrupt vector. This will fire an interrupt on each edge of our main PWM. we only use this once to synchronise the B sampling interrupt.
  //TIMSK1 |= _BV(OCIE1B);
  TIMSK1 |= _BV(OCIE1C); 
 //MsTimer2::start();
  
  //Set up PID Control Loop
  myPID.SetMode(MANUAL);
  myPID.SetSampleTime(PWM_PERIOD_MS);                //Since we run out PID every interupt cylce, we set the sample time (ms) to our PWM timer period 
  myPID.SetOutputLimits(0, ADC_SAMPLE_WINDOW_PWM_DUTY - 10);     //961max PWM, otherwise it will cut into the sample window TODO:dont leave hard coded

  myPID2.SetMode(MANUAL);
  myPID2.SetSampleTime(PWM_PERIOD_MS);                //Since we run out PID every interupt cylce, we set the sample time (ms) to our PWM timer period 
  myPID2.SetOutputLimits(0, ADC_SAMPLE_WINDOW_PWM_DUTY - 10);

  //TODO: dont leave this hard-coded

  myPID.SetTunings(kP, kI, kD);
  myPID2.SetTunings(kP, kI, kD);

  params.setpoint = 100.0;
  status.encoder_pos = 100;
  knob.write(status.encoder_pos);
  
  
}
