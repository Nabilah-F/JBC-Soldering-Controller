/* Copyright (c) 2017 timothyjager
   JBC-Soldering-Controller
   MIT License. See LICENSE file for details.
*/

//----------------Main Loop------------------------
void loop(void)
{
  if(fastDigitalRead(LPINA)){
    vSense0 = analogRead(CURRENT_SENSE0)*0.00488;//5.0*analogRead(CURRENT_SENSE0)/1024.0;// read mosfet current
    vSense1 = analogRead(CURRENT_SENSE1)*0.00488;
    //if(fastDigitalRead(LPINA)){
      vInDev = analogRead(VIN_SENSE)*0.00488;// read power supply voltage
      params.tipResistance0 = (vInDev/vSense0)*1.1798;// * (1200.0/7000.0) * (35100.0/5100.0);// work out resistance of tip
      params.tipResistance1 = (vInDev/vSense1)*1.1798;
      //params.iron = ;
      if(params.tipResistance0 > 2.80 && params.tipResistance0 < 3.75){
        params.iron = iron_T245;
      }
      else if(params.tipResistance0 > 1.75 && params.tipResistance0 < 2.65){
        params.iron = iron_T210;
      }
      else if(params.tipResistance0 > 2.65 && params.tipResistance0 < 2.80){
        ;
      }
      else {
        params.iron = iron_none;
      }
    //}
  }
  //static bool in_cradle;
  bool update_display_now = false;

  //read the encoder knob
  static int16_t knob_pos_last = 0;

  status.encoder_pos = knob.read(); //>> 2;  //divide by 2 to decrease sensitivity

  //Check if knob has changed
  if (status.encoder_pos != knob_pos_last)
  {
    //Dont allow it to go negative.
    if (status.encoder_pos < 0)
    {
      knob.write(0);
      status.encoder_pos = 0;
    }
    else if(status.encoder_pos > upper_set_limit)
    {
      knob.write(upper_set_limit);
      status.encoder_pos = upper_set_limit;
    }
    params.setpoint = status.encoder_pos;
    if (myPID.GetMode() == AUTOMATIC)
    {
      status.pid_setpoint = params.setpoint;
    }

    update_display_now = true;  //refresh the screen more quickly while adjusting the knob
  }
  knob_pos_last = status.encoder_pos;

  //When button is pressed toggle power on/off
  if (fastDigitalRead(ENC_BUTTON) == false)
  {
    //noInterrupts();
    if (iron_active == true)
    {
      iron_active = false;
      params.pid_mode = MANUAL;
      myPID.SetMode(params.pid_mode);
      myPID2.SetMode(params.pid_mode);
      status.pid_output = 0;
      status.pid_output2 = 0;
      status.pid_setpoint = 0;
      cradle_present=false;
    }
    else
    {
      iron_active = true;
      params.pid_mode = AUTOMATIC;
      myPID.SetMode(params.pid_mode);
      myPID2.SetMode(params.pid_mode);
      status.pid_setpoint = params.setpoint;
    }
    //interrupts();
    delay(200);
    while (fastDigitalRead(ENC_BUTTON) == false)
    delay(200);
  }


  //When on cradle power on/off
  if (fastDigitalRead(CRADLE_SENSOR) == false)
  {
    cradle_present = true; //we have deteected a cradle is being used. So from now on, we respond to on/off cradle events.
    //noInterrupts();
    if (params.pid_mode == AUTOMATIC)
    {
      params.pid_mode = MANUAL;
      myPID.SetMode(params.pid_mode);
      myPID2.SetMode(params.pid_mode);
      status.pid_output = 0;
      status.pid_setpoint = 0;
    }
    //interrupts();
  }
  else if(fastDigitalRead(CRADLE_SENSOR) == true && cradle_present && iron_active)
  {
      //noInterrupts();
      params.pid_mode = AUTOMATIC;
      myPID.SetMode(params.pid_mode);
      myPID2.SetMode(params.pid_mode);
      status.pid_setpoint = params.setpoint;
      //interrupts();
  }

  ProcessSerialComm();
  updateDisplay(0);
  updateLEDStatus();
}
