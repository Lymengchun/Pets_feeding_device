#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <FirebaseESP8266.h>
#include <Servo.h>

#include <HX711_ADC.h>
#if defined(ESP8266) || defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

//dout , sck
HX711_ADC FoodScale(10, 9);
HX711_ADC PetScale(5, 16);
int feednowID = 0;
int feedAmount = 0;
float oldFeed = 0.0;

float previousWeight = 0.0;

IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;

Servo myServo;
bool servoActivate = false;

// realtimeDB https://pets-feeding-backend-default-rtdb.asia-southeast1.firebasedatabase.app/
// secret 7gWEOFwuvkjWfGkM7BXqw28fm9A9H9xe6np1xjjG
// userUUID klsdjfalksdjfalksdf
String uuid;
unsigned long sendDataPrevMillis = 0;
int intValue;
float floatValue;
bool signupOK = false;
int dataChange = 0;
unsigned long servoMillis = 0;

void scanAP()
{
  int number = WiFi.scanNetworks();
  delay(200);
  DynamicJsonDocument doc(1024);
  for (int i = 0; i < number; i++)
  {
    Serial.println(WiFi.SSID(i));
    doc[i]["ap_ssid"] = WiFi.SSID(i);
    doc[i]["ap_signal"] = WiFi.RSSI(i);
    doc[i]["ap_security"] = WiFi.encryptionType(i) != ENC_TYPE_NONE;
  }
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void connectAP()
{
  if (server.hasArg("plain") == false)
  {
    server.send(200, "appication/json", "Body not received");
    return;
  }

  DynamicJsonDocument input(1024);
  deserializeJson(input, server.arg("plain"));
  String ssid_ap = input["ssid_ap"];
  String pass_ap = input["pass_ap"];
  WiFi.begin(ssid_ap, pass_ap);
  DynamicJsonDocument doc(1024);
  doc["status"] = WiFi.status();
  String output;
  serializeJson(doc, output);
  server.send(200, "appication/json", output);
}

void statusAP()
{
  DynamicJsonDocument doc(1024);
  doc["status"] = WiFi.status();
  if (WiFi.status() == 3)
  {
    Firebase.begin(&config, &auth);
  }
  String output;
  serializeJson(doc, output);
  server.send(200, "appication/json", output);
}

void adddUserUUID()
{
  if (server.hasArg("plain") == false)
  {
    server.send(200, "appication/json", "Body not received");
    return;
  }

  DynamicJsonDocument input(1024);
  deserializeJson(input, server.arg("plain"));
  String tmp = input["uuid"];
  uuid = tmp;
  DynamicJsonDocument doc(1024);
  doc["status"] = "Done";
  String output;
  Serial.println(uuid);
  serializeJson(doc, output);
  server.send(200, "appication/json", output);
}

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  config.api_key = "AIzaSyB-PsV7BAiunqTRB8eTyL_mJxDJoLh2218";
  config.database_url = "https://pets-feeding-backend-default-rtdb.asia-southeast1.firebasedatabase.app/";

  myServo.attach(2);
  if (!WiFi.isConnected())
  {
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP("Automatic Feeding Device");
    delay(100);
    server.on("/scan_ap", HTTP_GET, scanAP);
    server.on("/connect_ap", HTTP_POST, connectAP);
    server.on("/status_ap", HTTP_GET, statusAP);
    server.on("/addd_user_uuid", HTTP_POST, adddUserUUID);
    server.begin();
  }

  servoMillis = millis();
  Serial.begin(115200);
  Serial.println("Device Start");

  //Feed scale
  // FoodScale.begin();
  PetScale.begin();
  float calibrationValuePetWeight = 22000.0;      // uncomment this if you want to set the calibration value in the sketch
  float calibrationValue = 1266.0;  
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true;                 //set this to false if you don't want tare to be performed in the next step

  
  PetScale.start(stabilizingtime, _tare);
  if (PetScale.getTareTimeoutFlag())
  {
    Serial.println("PetScale Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else
  {
    PetScale.setCalFactor(calibrationValuePetWeight); // set calibration value (float)
    Serial.println("PetScale Startup is complete");
  }

  FoodScale.start(stabilizingtime, _tare);
  if (FoodScale.getTareTimeoutFlag())
  {
    Serial.println("FoodScale Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else
  {
    FoodScale.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("FoodScale Startup is complete");
  }

}

void loop()
{
  server.handleClient();
  if (!signupOK)
  {
    if (Firebase.signUp(&config, &auth, "", ""))
    {
      Serial.println("ok");
      signupOK = true;
    }
  }
  if ((millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    String prefix = "/FeedNow/";
    if (Firebase.RTDB.getJSON(&fbdo, prefix + uuid + "/id"))
    {
      bool tmp = servoActivate;
      if (fbdo.dataType() == "int")
      {
        int tmpId = fbdo.intData();
        if (tmpId != feednowID)
        {
          if (Firebase.RTDB.getJSON(&fbdo, prefix + uuid + "/amount"))
          {
            if (fbdo.dataType() == "int")
            {
              feedAmount = fbdo.intData();
              feednowID = tmpId;
              tmp = true;
              oldFeed = FoodScale.getData();
            }
          }
        }
      }
      servoActivate = tmp;
    }
  }

  if (servoActivate)
  {
    myServo.attach(2);
    myServo.write(180);
  }
  else
  {
    myServo.detach();
  }

  // weight = scale.get_units();
  // if (weight > 0.4)
  // {
  //   if (!scaleActivate)
  //   {
  //     scaleActivateTime = millis();
  //     scaleActivate = true;
  //   }
  //   averageValue.push(scale.get_units());
  // }
  // if (averageValue.average() > 0.4 && scaleActivate == true && millis() - scaleActivateTime > 5000)
  // {
  //   Serial.println(averageValue.average());
  //   if (Firebase.RTDB.setFloat(&fbdo, "/PetWeight/" + uuid, averageValue.average()))
  //   {
  //     Serial.println("PASSED");
  //     Serial.println("PATH: " + fbdo.dataPath());
  //     Serial.println("TYPE: " + fbdo.dataType());
  //   }
  //   else
  //   {
  //     Serial.println("FAILED");
  //     Serial.println("REASON: " + fbdo.errorReason());
  //   }
  //   scaleActivate = false;
  // }

  // get smoothed value from the dataset:
  if (PetScale.update() && PetScale.getData() > 0.4 && previousWeight == PetScale.getData())
  {
     if (Firebase.RTDB.setFloat(&fbdo, "/PetWeight/" + uuid, PetScale.getData()))
     {
        Serial.println(PetScale.getData());
     }
  }
  previousWeight = PetScale.getData();

  // get smoothed value from the dataset:
//  if (FoodScale.update())
//  {
//    Serial.printf("Food %f\n",FoodScale.getData());
//    if (FoodScale.getData() > oldFeed + feedAmount)
//    {
//      servoActivate = false;
//      Serial.println("servo stop");
//    }
//  }
}
