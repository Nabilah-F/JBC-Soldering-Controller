/* Copyright (c) 2017 timothyjager
   JBC-Soldering-Controller
   MIT License. See LICENSE file for details.
*/

//----------------Serial Comm------------------------

//process serial communications
//this function shoudl be called cyclically in the main loop
#define SERIAL_COMM_PERIOD_MS 250 //run every 200ms.
void ProcessSerialComm(void)
{
  static long next_millis = millis() + SERIAL_COMM_PERIOD_MS;  //determine the next time this function should activate
  static bool serial_active = false;

  //send-receive with processing if it's time
  if (millis() > next_millis)
  {
    //Check if there is data on the serial port. This indicated the host PC is sending us something.

    //Once we determine there is serial activity, we assume a host is connected, so begin streaming our packet data to the host. TODO: currently no way to detect if the host dissapears.
    if (Serial)
    {
      Serial.print(status.tip_temperature_c);
      Serial.print("\t");
      Serial.print(status.adcval);
      Serial.print("\t");
      Serial.print(status.tempCJ);
      Serial.print("\t");
      Serial.print(status.tip_temperature_c2);
      Serial.println("\t");
      //Serial.print(vSense0);
      //Serial.print("\t");
      Serial.println(params.tipResistance0);
     
    }
    next_millis += SERIAL_COMM_PERIOD_MS;  //set up our loop to run again in x ms
  }
}
