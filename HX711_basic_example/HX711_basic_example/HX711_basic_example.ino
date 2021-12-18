#include "HX711.h"
#include <AverageValue.h>
#define calibration_factor 7050.0 //This value is obtained using the SparkFun_HX711_Calibration sketch

HX711 scale;
float weight;
long scaleActivateTime = 0;
bool scaleActivate = false;
AverageValue<float> averageValue(50);
void setup() {
  Serial.begin(115200);
  Serial.println("HX711 scale demo");
  //dout , sck
  scale.begin(5, 16);
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0
}

void loop() {
  weight = scale.get_units()*0.73/2.205;
  if(weight > 0.4){
      if(!scaleActivate){
        scaleActivateTime = millis();
        scaleActivate = true;
      }
      averageValue.push(weight);
  }
  if(averageValue.average() > 0.4 && scaleActivate == true && millis() - scaleActivateTime > 3000){
      Serial.print(averageValue.average()); //scale.get_units() returns a float
      Serial.print(" kg"); //You can change this to kg but you'll need to refactor the calibration_factor
      Serial.println();
      scaleActivate = false;
  }
}
