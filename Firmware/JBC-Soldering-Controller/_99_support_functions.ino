/* Copyright (c) 2017 timothyjager
   JBC-Soldering-Controller
   MIT License. See LICENSE file for details.
*/

//----------------Support Functions------------------------


void updateLEDStatus(void)
{
    if (status.pid_setpoint == 0) //if off..
    {
      if(status.tip_temperature_c < coldTemp){ //if cold..
        pixels.SetPixelColor(0, RgbwColor(0, 0, 10)); // Blue
      }
      else {
        pixels.SetPixelColor(0, RgbwColor(0, 10, 10)); // ??
      }
      if(status.tip_temperature_c2 < coldTemp){ //if cold..
        pixels.SetPixelColor(1, RgbwColor(0, 0, 10)); // Blue
      }
      else {
        pixels.SetPixelColor(1, RgbwColor(0, 10, 10)); // ??
      }
    }
    else if(fastDigitalRead(CRADLE_SENSOR) == true && cradle_present) //if in cradle
    {
      pixels.SetPixelColor(0, RgbwColor(10, 0, 0)); // Green
      pixels.SetPixelColor(1, RgbwColor(10, 0, 0)); // Green
    }
    else if(status.pid_setpoint > 0) //heating
    {
      if(status.tip_temperature_c > (status.pid_setpoint - 10)) //if at temperature
      {
        pixels.SetPixelColor(0, RgbwColor(5, 10, 0)); // red/green
      }
      else
      {
        pixels.SetPixelColor(0, RgbwColor(0, 10, 0)); // red
      }
      if(status.tip_temperature_c2 > (status.pid_setpoint - 10)) //if at temperature
      {
        pixels.SetPixelColor(1, RgbwColor(5, 10, 0)); // red/green
      }
      else
      {
        pixels.SetPixelColor(1, RgbwColor(0, 10, 0)); // red
      }
    }
    else //something else???
    {
      pixels.SetPixelColor(0, RgbwColor(10)); // White
      pixels.SetPixelColor(1, RgbwColor(10)); // White
    }
    if (status.tip_temperature_c > 1000) //it must not be connected
    {
      pixels.SetPixelColor(0, RgbwColor(0)); // Black
    }
    if (status.tip_temperature_c2 > 1000) //it must not be connected
    {
      pixels.SetPixelColor(1, RgbwColor(0)); // Black
    }
    pixels.Show();
}

float CJtemp2volt (int temperature)
{
  float voltCJ;
  uint16_t tempCJ = temperature;
  int temp[31] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 
                  20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
                  30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40};
  float volt[31] = {0.397, 0.437, 0.477, 0.517, 0.557, 0.597, 0.637, 0.677, 0.718, 0.758,
                    0.798, 0.838, 0.879, 0.919, 0.960, 1.000, 1.041, 1.081, 1.122, 1.163, 
                    1.203, 1.244, 1.285, 1.326, 1.366, 1.407, 1.448, 1.489, 1.530, 1.571, 1.612};
  for (int i=1; i<=31; i++)
  {
    if ((tempCJ>=temp[i-1]) && (tempCJ <=temp[i]))
    {
      voltCJ = ((tempCJ-temp[i-1])/(temp[i]-temp[i-1]))*(volt[i]-volt[i-1])+volt[i-1];
    }
  }
  return voltCJ;
}

int ironvolt2temp (float voltage)
{
  uint16_t tempTC = 0;
  float voltTC = voltage;
  int temp[45] = {10, 20, 30, 40,
                   50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190,
                   200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300, 310, 320, 340, 
                   350, 360, 370, 380, 390, 400, 410, 420, 430, 440, 450};
                
  float volt[45] = {0.397, 0.798, 1.203, 1.612,
                     2.023, 2.436, 2.851, 3.267, 3.682, 4.096, 4.509, 4.920, 5.328, 5.735, 
                     6.540, 6.941, 7.340, 7.739, 8.138, 8.539, 8.940, 9.343, 9.747, 10.153,    
                     10.561, 10.971, 11.382, 11.795, 12.209, 12.624, 13.040, 13.457, 13.874,  
                     14.293, 14.713, 15.133, 15.554, 15.975, 16.397, 16.820, 17.243, 17.667, 18.091, 18.516};
  if(voltTC>volt[45])tempTC=9999;
  for (int i=1; i<=45; i++)
  {
    if ((voltTC>=volt[i-1]) && (voltTC <=volt[i]))
    {
      tempTC = ((voltTC-volt[i-1])/(volt[i]-volt[i-1]))*(temp[i]-temp[i-1])+temp[i-1];
    }
  }
  return tempTC; 
}
