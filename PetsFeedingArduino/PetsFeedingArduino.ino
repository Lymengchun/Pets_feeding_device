#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <FirebaseESP8266.h>
//#include "HX711.h"

//HX711 scale;

IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;
// realtimeDB https://pets-feeding-backend-default-rtdb.asia-southeast1.firebasedatabase.app/
// secret 7gWEOFwuvkjWfGkM7BXqw28fm9A9H9xe6np1xjjG
// userUUID klsdjfalksdjfalksdf
String uuid;
unsigned long sendDataPrevMillis = 0;
int intValue;
float floatValue;
bool signupOK = false;

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
//0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
//
//1 : WL_NO_SSID_AVAILin case configured SSID cannot be reached
//
//3 : WL_CONNECTED after successful connection is established
//
//4 : WL_CONNECT_FAILED if connection failed
//
//6 : WL_CONNECT_WRONG_PASSWORD if password is incorrect
//
//7 : WL_DISCONNECTED if module is not configured in station mode
void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  config.api_key = "AIzaSyB-PsV7BAiunqTRB8eTyL_mJxDJoLh2218";
  config.database_url = "https://pets-feeding-backend-default-rtdb.asia-southeast1.firebasedatabase.app/";

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

  //  Serial.println("HX711 scale demo");

  //  scale.begin(14, 12);
  //  scale.set_scale(-7050.0); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  //  scale.tare();             //Assuming there is no weight on the scale at start up, reset the scale to 0
  //
  //  Serial.println("Readings:");
}

void loop()
{
  server.handleClient();
  //  Serial.print("Reading: ");
  //  Serial.print(scale.get_units(), 1); //scale.get_units() returns a float
  //  Serial.print(" lbs");               //You can change this to kg but you'll need to refactor the calibration_factor
  //  Serial.println();
  if (!signupOK)
  {
    if (Firebase.signUp(&config, &auth, "", ""))
    {
      Serial.println("ok");
      signupOK = true;
    }
    else
    {
      Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }
  }
  if ((millis() - sendDataPrevMillis > 10000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    String prefix = "/FeedNow/";
    if (Firebase.RTDB.getInt(&fbdo, prefix + "YbD9pFzWlAQhNMFhPtJRj782im23"))
    {
      if (fbdo.dataType() == "int")
      {
        intValue = fbdo.intData();
        Serial.println(intValue);
      }
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.getFloat(&fbdo, "/test/float"))
    {
      if (fbdo.dataType() == "float")
      {
        floatValue = fbdo.floatData();
        Serial.println(floatValue);
      }
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }
  }
}
