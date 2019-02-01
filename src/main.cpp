#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <max6675.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include "Timer.h"

#define b_led 2 // 1 for ESP-01, 2 for ESP-12
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
Timer t;
boolean busy = false;

#define DHTPIN D3 // Pin which is connected to the DHT sensor.

#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);
float pfDew, pfHum, pfTemp, pfVcc;
int ktcSO = 12;
int ktcCS = 13;
int ktcCLK = 14;
float a0value;
float rawvalue = 0;

void readDHT()
{

  // Delay between measurements.
  // delay(delayMS);
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature))
  {
    Serial.println("Error reading temperature!");
  }
  else
  {
    Serial.print("Temperature: ");
    Serial.print(event.temperature);
    Serial.println(" *C");
    pfTemp = event.temperature;
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity))
  {
    Serial.println("Error reading humidity!");
  }
  else
  {
    Serial.print("Humidity: ");
    Serial.print(event.relative_humidity);
    Serial.println("%");
    pfHum = event.relative_humidity;
  }

  // tempC = sensors.getTempC(t);
}
void checkin()
{
  busy = true;
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject &JSONencoder = JSONbuffer.createObject();
  JSONencoder["mac"] = WiFi.macAddress();
  JSONencoder["password"] = "";
  JSONencoder["ip"] = WiFi.localIP().toString();
  // JSONencoder["t"] = pfTemp;
  // JSONencoder["h"] = pfHum;
  // JsonObject &dht = JSONencoder.createNestedObject("dhtvalue");
  //  dht["t"] = pfTemp;
  // dht["h"] = pfHum;

  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println(JSONmessageBuffer);
  // put your main code here, to run repeatedly:
  HTTPClient http; //Declare object of class HTTPClient

  http.begin("http://endpoint.pixka.me:8081/checkin"); //Specify request destination
  http.addHeader("Content-Type", "application/json");  //Specify content-type header
  http.addHeader("Authorization", "Basic VVNFUl9DTElFTlRfQVBQOnBhc3N3b3Jk");

  int httpCode = http.POST(JSONmessageBuffer); //Send the request
  String payload = http.getString();           //Get the response payload
  Serial.print(" Http Code:");
  Serial.println(httpCode); //Print HTTP return code
  Serial.print(" Play load:");
  Serial.println(payload); //Print request response payload

  http.end(); //Close connection
  busy = false;
}
int getPort(String p)
{
  if (p == "D1")
  {
    return D1;
  }
  else if (p == "D2")
  {
    return D2;
  }
  /*  else if (p == "D3")
  {
    return D3;
  }

  else if (p == "D4")
  {
    return D4;
  }
  */
  else if (p == "D5")
  {
    return D5;
  }
  else if (p == "D6")
  {
    return D6;
  }
  else if (p == "D7")
  {
    return D7;
  }
  else if (p == "D8")
  {
    return D8;
  }
}
void status()
{
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["status"] = busy;
  char jsonChar[500];
  root.printTo((char *)jsonChar, root.measureLength() + 1);
  server.send(200, "application/json", jsonChar);
}
JsonObject &prepareResponse(JsonBuffer &jsonBuffer)
{
  JsonObject &root = jsonBuffer.createObject();
  root["t"] = pfTemp;
  root["h"] = pfHum;
  root["ip"] = WiFi.macAddress();
  return root;
}
void DHTtoJSON()
{
  digitalWrite(b_led, LOW);
  readDHT();
  digitalWrite(b_led, HIGH);
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject &json = prepareResponse(jsonBuffer);
  char jsonChar[100];
  json.printTo((char *)jsonChar, json.measureLength() + 1);
  server.send(200, "application/json", jsonChar);
}
void run()
{
  busy = true;
  Serial.println("Run");
  String p = server.arg("port");
  String v = server.arg("value");
  String d = server.arg("delay");
  String w = server.arg("wait");
  Serial.println("Port: " + p + " value : " + v + " delay: " + d);
  int value = v.toInt();

  //int d = server.arg("delay").toInt();
  int port = getPort(p);
  digitalWrite(port, value);
  server.send(200, "application/json", "ok");
  delay(d.toInt() * 1000);
  digitalWrite(port, !value);
  delay(w.toInt() * 1000);
  busy = false;
}

void setup()
{
  Serial.begin(9600);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(b_led, OUTPUT);
  //pinMode(D3, OUTPUT);
  // pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);

  // connect();
  WiFiMulti.addAP("Sirifarm", "0932154741");
  WiFiMulti.addAP("pksy", "04qwerty");
  WiFiMulti.addAP("SP", "04qwerty");
  WiFiMulti.addAP("SP1", "04qwerty");

  while (WiFiMulti.run() != WL_CONNECTED) //รอการเชื่อมต่อ
  {
    delay(500);
    Serial.print(".");
  }

  server.on("/run", run);
  server.on("/status", status);
  server.on("/dht", DHTtoJSON);

  //  server.on("/info", info);
  server.begin(); //เปิด TCP Server
  Serial.println("Server started");
  Serial.println(WiFi.localIP()); // แสดงหมายเลข IP ของ Server
  String mac = WiFi.macAddress();
  Serial.println(mac); // แสดงหมายเลข IP ของ Server
  t.every(60000, checkin);
}

void loop()
{
  server.handleClient();
  t.update();
  digitalWrite(b_led, 0);
  delay(500);
  digitalWrite(b_led, 1);
  delay(500);

  // put your main code here, to run repeatedly:
}