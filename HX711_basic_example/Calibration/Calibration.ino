#include "HX711.h"
#include <AverageValue.h>
#define calibration_factor 7050.0 //This value is obtained using the SparkFun_HX711_Calibration sketch

#define LOADCELL_DOUT_PIN  4
#define LOADCELL_SCK_PIN  3

HX711 scale;
float weight = 0;
AverageValue<int> averageValue(50);
bool scaleActivate = false;
void setup() {
  Serial.begin(9600);
  Serial.println("HX711 scale demo");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0
  Serial.println("Readings:");
}

void loop() {
  weight = scale.get_units()*0.73/2.205;
  if(weight > 0.4 ){
    
  }else{
    
  }
}
