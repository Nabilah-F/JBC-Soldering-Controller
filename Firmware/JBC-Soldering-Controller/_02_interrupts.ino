/* Copyright (c) 2017 timothyjager
   JBC-Soldering-Controller
   MIT License. See LICENSE file for details.
*/

//----------------Interrupts------------------------

//This interrupt is set to fire approximately 1.2ms before the PWM output turn on. It is configured to allow just enough time to sample the ADC while the
//power to the heater is turned off. This means that our max PWM has to be limited to ensure there is a long enough sample window.
//the interupt actually fires twice. once at the beginning of our interval and once at the end. we use this to our advantage by sampling the result of the
//previous sample at each interrupt and then toggling the sample type back and forth between ADC and internal temp.
ISR(TIMER1_COMPB_vect)
{
  ;
}
  //mosfet_states = mosfet_states ^ 0x01;
  //mosfet_stateB = !mosfet_stateB;
  //static bool one_shot = false;*/
  //If we have never run this code before, then run it just once
  /*if (!one_shot)
  {
    MsTimer2::start();
    TIMSK1 = 0;                 //Disable interrupts on timer 1
    one_shot = true; 
    low_mosfet_state = false;
    TIFR1 |= _BV(OCF1C);
    TIMSK1 = _BV(OCIE1C); 
  }*/
  //if(mosfet_states == 0){
  /*if(mosfet_stateB == 0){
    low_mosfet_state = 0;
    fastDigitalWrite(LOWMOS_0, low_mosfet_state);
  }
  else{
    low_mosfet_state = 1;
    fastDigitalWrite(LOWMOS_0, low_mosfet_state);
  }*/
  /*if(status.pid_output2 == 0 && status.pid_output == 0){
    low_mosfet_state = 0;
    fastDigitalWrite(LOWMOS_0, 0);
  }*/
//}


//This should only run at the start of the program, then it disables itself
//It's used to start the B interupt at the right time
ISR(TIMER1_COMPA_vect)
{
  ;
}
  //mosfet_states = mosfet_states ^ 0x02;
  //mosfet_stateA = !mosfet_stateA;*/
  /*static bool one_shot = false;
  //If we have never run this code before, then run it just once
  if (!one_shot)
  {
    TIFR1 |= _BV(OCF1B);        //clear the Timer 1 B interrupt flag so it doesn't fire right after we enable it. We only want to detect new interrupts, not old ones.
    TIMSK1 = _BV(OCIE1B);       //Enable only interrupt B, This also disables A since A has done it's one and only job of synchronizing the B interrupt.
    one_shot = true;            //set oneshot to prevent this code from ever running again. It shouldn't try to since this interrupt should now be disable. 
    
  }*/
  //fastDigitalWrite(LOWMOS_0, mosfet_stateA);
  //if(mosfet_states == 0){
  /*if(mosfet_stateA == 0){
    low_mosfet_state = 0;
    fastDigitalWrite(LOWMOS_0, low_mosfet_state);
  }
  else{
    low_mosfet_state = 1;
    fastDigitalWrite(LOWMOS_0, low_mosfet_state);
  }*/
  /*if(status.pid_output2 == 0 && status.pid_output == 0){
    low_mosfet_state = 0;
    fastDigitalWrite(LOWMOS_0, 0);
  }*/
//}

ISR(TIMER1_COMPC_vect)
{
  /*//This is to turn on and off the low mosfet.
  low_mosfet_state = !low_mosfet_state;
  fastDigitalWrite(LOWMOS_0, low_mosfet_state);
  //fastDigitalWrite(LOWMOS_1, low_mosfet_state);
  //state = !state;*/
  flash();
}

void flash() {
  float adcvolt, finalvolt;
  int tempTC;  
  static int interrupt_count = 0;

    fastDigitalWrite(DBG_OUTPUT, HIGH);
  
    if (interrupt_count == 0) 
    {            
      fastDigitalWrite(CS, LOW);
      SPI.transfer(0x08);
      while (digitalRead(SPI_MISO)==HIGH)
      {;};
      delayMicroseconds(1);
      status.adcval = SPI.transfer(0x00);
      status.adcval = (status.adcval<<8)|SPI.transfer(0x00);
      delayMicroseconds(1);
      adcvolt = (16.0*status.adcval)/(32767.0); // converting temperature adc counts to voltage    
      finalvolt = adcvolt+status.voltCJ;
      tempTC = ironvolt2temp(finalvolt);   
      if(params.iron == iron_T210)tempTC = tempTC * 2.87 - 53.0; 
      //if(params.iron == iron_T245)tempTC = tempTC * 1.0 - 53.0;
      status.tip_temperature_c = tempTC;
      
      //Compute PID
      myPID.Compute();
      //Update PWM output
      if (status.tip_temperature_c > 500) //If there is nothing connected...
      {
        Timer1.pwm(LPINA, 0); //100% = 1023 //...make sure that nothing is output. (this protects against the thermocouple disconnects)
      }
      else
      {
        if(status.pid_output == 0){
          Timer1.pwm(LPINA, 1); //100% = 1023
        }
        else {
          Timer1.pwm(LPINA, status.pid_output); //100% = 1023
        }
        //Timer1.pwm(LPINA, status.pid_output); //100% = 1023
        if(disable_simultaneous_output)Timer1.pwm(LPINB, 0);//set other output to 0%
      }
      
      fastDigitalWrite(CS,HIGH);      
      interrupt_count++;
    }
    
    else 
    {    
      fastDigitalWrite(CS, LOW);
      SPI.transfer(0x08);
      while (digitalRead(SPI_MISO)==HIGH)
      {;};
      delayMicroseconds(1);
      status.adcval = SPI.transfer(0x00);
      status.adcval = (status.adcval<<8)|SPI.transfer(0x00);
      delayMicroseconds(1);
      adcvolt = (16.0*status.adcval)/(32767.0); // converting temperature adc counts to voltage
      finalvolt = adcvolt+status.voltCJ;
      tempTC = ironvolt2temp(finalvolt);
      if(params.iron == iron_T210)tempTC = tempTC * 2.87 - 53.0; 
      //if(params.iron == iron_T245)tempTC = tempTC * 1.0 - 53.0;
      status.tip_temperature_c2 = tempTC;
      
      //Compute PID
      myPID2.Compute();
      //Update PWM output
      if (status.tip_temperature_c2 > 500) //If there is nothing connected...
      {
        Timer1.pwm(LPINB, 0); //100% = 1023 //...make sure that nothing is output. (this protects against the thermocouple disconnects)
      }
      else
      {
        if(status.pid_output2 == 0){
          Timer1.pwm(LPINB, 1); //100% = 1023
        }
        else {
          Timer1.pwm(LPINB, status.pid_output2); //100% = 1023
        }
        if(disable_simultaneous_output)Timer1.pwm(LPINA, 0);//set other output to 0%
      }
      fastDigitalWrite(CS,HIGH);    
      interrupt_count=0;
    }  
  /*if(status.pid_output2 == 0 && status.pid_output == 0){
    low_mosfet_state = 0;
    fastDigitalWrite(LOWMOS_0, 0);
  }*/
  
  //fastDigitalWrite(LOWMOS_0, true);
  //fastDigitalWrite(LOWMOS_1, true);
  fastDigitalWrite(DBG_OUTPUT, LOW);
}
