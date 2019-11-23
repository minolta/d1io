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
// #include "Timer.h"
#include <Ticker.h>
#include <Wire.h>
#include "SSD1306.h"
long uptime = 0;
long otatime = 0;
long checkintime = 0;
// #include <ESP8266Ping.h>
// #define useI2C 1
#define ioport 7
String name = "d1io";
String version = "50";
// SSD1306 display(0x3C, D2, D1);
//D2 = SDA  D1 = SCL
// String hosttraget = "192.168.88.9:2222";
String hosttraget = "fw1.pixka.me:2222";
// String otahost = "192.168.88.9";
String otahost = "fw1.pixka.me";
String updateString = "/espupdate/d1io/" + version;
SSD1306 display(0x3C, RX, TX);
String message = "";
class Dhtbuffer
{
public:
  float h;
  float t;
  int count;
};

Dhtbuffer dhtbuffer;
Ticker flipper;
#define b_led 2 // 1 for ESP-01, 2 for ESP-12
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
// Timer t, t2;
boolean busy = false;
int count = 0;
#define DHTPIN D3 // Pin which is connected to the DHT sensor.

#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);
float pfDew, pfHum, pfTemp, pfVcc;
int ktcSO = 12;
int ktcCS = 13;
int ktcCLK = 14;
float a0value;
float rawvalue = 0;
class Portio
{
public:
  int port;
  int value;
  int delay;
  int waittime;
  int run = 0;
  String closetime;
  String name;
  Portio *n;
  Portio *p;
};

Portio ports[ioport];
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

void ota()
{
  Serial.println("CALL " + otahost + " " + updateString);
  t_httpUpdate_return ret = ESPhttpUpdate.update(otahost, 8080, updateString, version);
  // t_httpUpdate_return ret = ESPhttpUpdate.update("http://fw-dot-kykub-161406.appspot.com", 80, "/espupdate/d1proio/" + version, version);
  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.println("[update] Update failed.");
    message = "Update failed";
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("[update] Update no Update.");
    message = "Device already updated";
    break;
  case HTTP_UPDATE_OK:
    Serial.println("[update] Update ok."); // may not called we reboot the ESP
    break;
  }
}
void disp_data(void)
{
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(20, 1, "D1 io ");
  display.drawString(80, 1, "IP" + WiFi.localIP().toString());
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(1, 20, "H:");

  display.display();
}
void trytoota()
{
  String re = "";
  t_httpUpdate_return ret = ESPhttpUpdate.update("192.168.88.9", 8080, "/espupdate/d1io/1");
  // t_httpUpdate_return ret = ESPhttpUpdate.update("http://fw-dot-kykub-161406.appspot.com", 80, "/espupdate/d1proio/" + version, version);
  Serial.println(ret);
  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.println("[update] Update failed.");
    re = "Update failed";
    break;
  case HTTP_UPDATE_NO_UPDATES:

    Serial.println("[update] Update no Update.");
    re = "No update";
    break;
  case HTTP_UPDATE_OK:
    Serial.println("[update] Update ok."); // may not called we reboot the ESP
    re = "update ok";
    break;
  }
  StaticJsonDocument<1000> doc;
  JsonObject portsobj = doc.createNestedObject("ports");
  for (int i = 0; i < ioport; i++)
  {

    JsonObject o = portsobj.createNestedObject(new String(i));
    o["port"] = ports[i].port;
    o["closetime"] = ports[i].closetime;
    o["delay"] = ports[i].delay;
  }
  doc["name"] = name;
  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["ssid"] = WiFi.SSID();
  doc["version"] = version;
  readDHT();
  doc["h"] = pfHum;
  doc["t"] = pfTemp;
  doc["uptime"] = uptime;
  doc["updateresult"] = re;
  char jsonChar[1000];
  serializeJsonPretty(doc, jsonChar, 1000);
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Headers", "application/json");
  // 'Access-Control-Allow-Headers':'application/json'
  server.send(200, "application/json", jsonChar);
}
void status()
{

  StaticJsonDocument<1000> doc;
  JsonObject portsobj = doc.createNestedObject("ports");
  for (int i = 0; i < ioport; i++)
  {

    JsonObject o = portsobj.createNestedObject(new String(i));
    o["port"] = ports[i].port;
    o["closetime"] = ports[i].closetime;
    o["delay"] = ports[i].delay;
  }
  doc["name"] = name;
  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["ssid"] = WiFi.SSID();
  doc["version"] = version;
  readDHT();
  doc["h"] = pfHum;
  doc["t"] = pfTemp;
  doc["uptime"] = uptime;
  doc["message"] = message;
  char jsonChar[1000];
  serializeJsonPretty(doc, jsonChar, 1000);
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Headers", "application/json");
  // 'Access-Control-Allow-Headers':'application/json'
  server.send(200, "application/json", jsonChar);
}
void setclosetime()
{

  int s = server.arg("time").toInt();
  // timetocount = s;
  digitalWrite(D5, 1);

  String closetime = server.arg("closetime");
  ports[2].delay = s;
  ports[2].value = 1;
  ports[2].closetime = closetime;
  StaticJsonDocument<500> doc;

  doc["run"] = "ok";
  doc["countime"] = s;
  doc["closeat"] = closetime;
  char jsonChar[100];
  serializeJsonPretty(doc, jsonChar, 100);
  server.send(200, "application/json", jsonChar);
}

void checkin()
{
  int connectcount = 0;
  while (WiFiMulti.run() != WL_CONNECTED) //รอการเชื่อมต่อ
  {
    delay(500);
    Serial.print(".");
    connectcount++;
    if (connectcount >= 20)
    {
      if (!WiFi.reconnect())
        // ESP.restart();
        return;
    }
  }
  busy = true;
  StaticJsonDocument<500> doc;
  doc["mac"] = WiFi.macAddress();
  doc["password"] = "";
  doc["ip"] = WiFi.localIP().toString();
  doc["uptime"]=uptime;
  char JSONmessageBuffer[300];
  serializeJsonPretty(doc, JSONmessageBuffer, 300);
  Serial.println(JSONmessageBuffer);
  // put your main code here, to run repeatedly:
  HTTPClient http; //Declare object of class HTTPClient
  String h = "http://" + hosttraget + "/checkin";
  http.begin(h);                                      //Specify request destination
  http.addHeader("Content-Type", "application/json"); //Specify content-type header
  http.addHeader("Authorization", "Basic VVNFUl9DTElFTlRfQVBQOnBhc3N3b3Jk");

  int httpCode = http.POST(JSONmessageBuffer); //Send the request
  String payload = http.getString();           //Get the response payload
  Serial.print(" Http Code:");
  Serial.println(httpCode); //Print HTTP return code
  if (httpCode == 200)
  {
    Serial.print(" Play load:");
    Serial.println(payload); //Print request response payload
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    JsonObject obj = doc.as<JsonObject>();
    name = obj["pidevice"]["name"].as<String>();
  }

  http.end(); //Close connection
  busy = false;
  // ota();
}
void setport()
{
  ports[0].port = D1;
  ports[0].name = "D1";
  ports[1].port = D2;
  ports[1].name = "D2";

  ports[2].port = D5;
  ports[2].name = "D5";

  ports[3].port = D6;
  ports[3].name = "D6";

  ports[4].port = D7;
  ports[4].name = "D7";

  ports[5].port = D8;
  ports[5].name = "D8";

  // ports[6].port = D3;
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
  else if (p == "D3")
  {
    return D3;
  }

  else if (p == "D4")
  {
    return D4;
  }

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

void DHTtoJSON()
{
  Serial.println("Read DHT");
  digitalWrite(b_led, LOW);

  if (dhtbuffer.count <= 0)
  {
    readDHT();
    dhtbuffer.count = 60;
  }
  digitalWrite(b_led, HIGH);
  StaticJsonDocument<500> doc;
  doc["t"] = pfTemp;
  doc["h"] = pfHum;
  doc["ip"] = WiFi.macAddress();
  char jsonChar[500];
  serializeJsonPretty(doc, jsonChar, 500);
  server.send(200, "application/json", jsonChar);
  Serial.println("End read dht");
}
void addTorun(int port, int delay, int value, int wait)
{

  for (int i = 0; i < ioport; i++)
  {
    if (ports[i].port == port)
    {
      // if (!ports[i].run)
      // {
      ports[i].value = value;
      ports[i].delay = delay;
      ports[i].waittime = wait;
      ports[i].run = 1;
      digitalWrite(ports[i].port, value);
      Serial.println("Set port");
    }
  }
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
  addTorun(port, d.toInt(), v.toInt(), w.toInt());
  // digitalWrite(port, value);
  // server.send(200, "application/json", "ok");
  StaticJsonDocument<500> doc;
  doc["status"] = "ok";
  doc["port"] = p;
  doc["runtime"] = d;
  doc["value"] = value;
  doc["mac"] = WiFi.macAddress();
  doc["ip"] = WiFi.localIP().toString();
  char jsonChar[500];
  serializeJsonPretty(doc, jsonChar, 500);
  server.send(200, "application/json", jsonChar);
  // delay(d.toInt() * 1000);
  // digitalWrite(port, !value);
  // delay(w.toInt() * 1000);
  // busy = false;
}
void ssid()
{
  StaticJsonDocument<500> doc;
  doc["ssid"] = WiFi.SSID();

  char jsonChar[500];
  serializeJsonPretty(doc, jsonChar, 500);
  server.send(200, "applic ation/json", jsonChar);
}
void flip()
{
  uptime++;
  otatime++;
  checkintime++;
  int state = digitalRead(b_led); // get the current state of GPIO1 pin
  digitalWrite(b_led, !state);    // set pin to the opposite state
  dhtbuffer.count--;
  for (int i = 0; i < ioport; i++)
  {
    //  Serial.println("Check port " + ports[i].port);
    if (ports[i].delay > 0)
    {
      ports[i].delay--;
      Serial.print("Port  ");
      Serial.println(ports[i].port);
      Serial.print(" Delay ");
      Serial.println(ports[i].delay);
      if (ports[i].delay == 0)
      {
        ports[i].run = 0;
        digitalWrite(ports[i].port, !ports[i].value);
        Serial.println("End job");
      }
    }

    if (ports[i].delay == 0)
      ports[i].run = 0;
  }
}
void setupport()
{
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(b_led, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);

  digitalWrite(D1,0);
  digitalWrite(D2,0);
  digitalWrite(D5,0);
  digitalWrite(D6,0);
  digitalWrite(D7,0);
  digitalWrite(D8,0);
}
void setup()
{
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  

  setport();
  // WiFiMulti.addAP("forpi3", "04qwerty");
  WiFiMulti.addAP("forpi", "04qwerty");
  WiFiMulti.addAP("forpi2", "04qwerty");
  WiFiMulti.addAP("Sirifarm", "0932154741");
  WiFiMulti.addAP("test", "12345678");
  WiFiMulti.addAP("farm", "12345678");
  WiFiMulti.addAP("pksy", "04qwerty");
  WiFiMulti.addAP("ky_MIFI", "04qwerty");

  int co = 0;
  while (WiFiMulti.run() != WL_CONNECTED) //รอการเชื่อมต่อ
  {
    delay(500);
    setupport();
    Serial.print(".");
    co++;
    if (co > 40)
    {
      ESP.restart();
    }
  }
  checkin();
  ota();
  Serial.println(WiFi.localIP()); // แสดงหมายเลข IP ของ Server
  String mac = WiFi.macAddress();
  Serial.println(mac); // แสดงหมายเลข IP ของ Server
  server.on("/run", run);
  server.on("/ssid", ssid);
  server.on("/status", status);
  server.on("/dht", DHTtoJSON);
  server.on("/ota", trytoota);
  server.on("/",status);

  server.on("/setclosetime", setclosetime);
  // server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  // server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  //  server.on("/info", info);
  server.begin(); //เปิด TCP Server
  Serial.println("Server started");
#ifdef useI2C
  display.init();
  display.flipScreenVertically();
  disp_data();
#endif

  // t.every(60000, checkin);
  flipper.attach(1, flip);
  dht.begin();
}
void loop()
{
  server.handleClient();
  // t.update();

  if (checkintime > 600)
  {
    checkintime = 0;
    checkin();
  }
  if (otatime > 60)
  {
    otatime = 0;
    ota();
  }
}