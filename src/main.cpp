#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
// #include <max6675.h>
// #include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
// #include "Timer.h"
#include <Ticker.h>
#include <Wire.h>
#include <EEPROM.h>
#define ADDR 100

// #include "SSD1306.h"
long uptime = 0;
long otatime = 0;
long checkintime = 0;
long timetoreaddht = 0;
long porttrick = 0;
long readdhttime = 0;
int apmode = 0;
// #include <ESP8266Ping.h>
// #define useI2C 1
#define ioport 7
String name = "d1io";
String type = "D1IO";
String version = "57";
long counttime = 0;
// SSD1306 display(0x3C, D2, D1);
//D2 = SDA  D1 = SCL
// String hosttraget = "192.168.88.9:2222";
String hosttraget = "fw1.pixka.me:2222";
// String otahost = "192.168.88.9";
String otahost = "fw1.pixka.me";
String updateString = "/espupdate/d1io/" + version;
// SSD1306 display(0x3C, RX, TX);
String message = "";
class Dhtbuffer
{
public:
  float h;
  float t;
  int count;
};

class Wifidata
{
public:
  char ssid[50];
  char password[50];
};
Wifidata wifidata;
String p;
String s;

Dhtbuffer dhtbuffer;
Ticker flipper;
#define b_led 2 // 1 for ESP-01, 2 for ESP-12
ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server(80);
// Timer t, t2;
boolean busy = false;
int count = 0;
#define DHTPIN D3     // Pin which is connected to the DHT sensor.
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
// #include <ESPAsyncWebServer.h>
// AsyncWebServer aserver(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP WIFI </title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    SSID: <input type="text" name="ssid">
    PASSWORD: <input type="password" name="password">
    <input type="submit" value="Submit">
  </form><br>
</body></html>)rawliteral";

void setAPMode()
{
  String mac = WiFi.macAddress();
  WiFi.softAP("ESP" + mac, "12345678");
  IPAddress IP = WiFi.softAPIP();
  Serial.println(IP.toString());
  apmode = 1;
}
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
    dhtbuffer.t = pfTemp;
    dhtbuffer.count = 120; //update buffer life time
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
    dhtbuffer.h = pfHum;
    dhtbuffer.count = 120; //update buffer life time
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
// void disp_data(void)
// {
//   display.clear();
//   display.setTextAlignment(TEXT_ALIGN_CENTER);
//   display.setFont(ArialMT_Plain_10);
//   display.drawString(20, 1, "D1 io ");
//   display.drawString(80, 1, "IP" + WiFi.localIP().toString());
//   display.setTextAlignment(TEXT_ALIGN_LEFT);
//   display.setFont(ArialMT_Plain_16);
//   display.drawString(1, 20, "H:");

//   display.display();
// }
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

void get()
{
  String ssd = server.arg("ssid");
  String password = server.arg("password");

  Serial.print("SSD ");
  Serial.println(ssd);
  Serial.print("password ");
  Serial.println(password);

  if (ssd != NULL)
    ssd.toCharArray(wifidata.ssid, 50);

  if (password != NULL)
    password.toCharArray(wifidata.password, 50);

  Serial.println("Set ok");
  EEPROM.put(ADDR, wifidata);
  EEPROM.commit();
  server.send(200, "application/json", "SSID" + ssd + " P:" + password);
}
void status()
{
  StaticJsonDocument<1000> doc;
  JsonObject portsobj = doc.createNestedObject("ports");

  doc["name"] = name;
  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["ssid"] = WiFi.SSID();
  doc["version"] = version;
  doc["h"] = pfHum;
  doc["t"] = pfTemp;
  doc["uptime"] = uptime;
  doc["message"] = message;
  doc["dhtbuffer.time"] = dhtbuffer.count;
  doc["porttrick"] = porttrick;
  doc["counttime"] = counttime;
  doc["savessid"] = wifidata.ssid;
  doc["savepassword"] = wifidata.password;

  for (int i = 0; i < ioport; i++)
  {
    JsonObject o = portsobj.createNestedObject(new String(i));
    o["port"] = ports[i].port;
    o["closetime"] = ports[i].closetime;
    o["delay"] = ports[i].delay;
  }
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
    Serial.print("#");
    connectcount++;
    if (connectcount > 20)
      return;
  }
  busy = true;
  StaticJsonDocument<500> doc;
  doc["mac"] = WiFi.macAddress();
  doc["password"] = "";
  doc["ip"] = WiFi.localIP().toString();
  doc["uptime"] = uptime;
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
    dhtbuffer.count = 120;
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

  if (delay > counttime)
    counttime = delay;
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
void setwifi()
{
  server.send(200, "text/html", index_html);
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
  uptime++;      //เวลา run
  otatime++;     //เพิ่มการ update
  checkintime++; //เพิ่มการนับ
  porttrick++;   //บอกว่า 1 วิละ
  readdhttime++; //บอกเวลา สำหรับอ่าน DHT

  if (counttime > 0)
    counttime--;

  // get the current state of GPIO1 pin
  digitalWrite(b_led, !digitalRead(b_led)); // set pin to the opposite state
  dhtbuffer.count--;
}
void portcheck()
{
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

  digitalWrite(D1, 0);
  digitalWrite(D2, 0);
  digitalWrite(D5, 0);
  digitalWrite(D6, 0);
  digitalWrite(D7, 0);
  digitalWrite(D8, 0);
}
void reset()
{
  StaticJsonDocument<1000> doc;
  JsonObject portsobj = doc.createNestedObject("ports");

  doc["name"] = name;
  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["ssid"] = WiFi.SSID();
  doc["version"] = version;
  doc["h"] = pfHum;
  doc["t"] = pfTemp;
  doc["uptime"] = uptime;
  doc["message"] = message;
  doc["dhtbuffer.time"] = dhtbuffer.count;
  doc["porttrick"] = porttrick;
  doc["counttime"] = counttime;
  doc["savessid"] = wifidata.ssid;
  doc["savepassword"] = wifidata.password;
  doc["reset"] = "OK";
  for (int i = 0; i < ioport; i++)
  {
    JsonObject o = portsobj.createNestedObject(new String(i));
    o["port"] = ports[i].port;
    o["closetime"] = ports[i].closetime;
    o["delay"] = ports[i].delay;
  }
  char jsonChar[1000];
  serializeJsonPretty(doc, jsonChar, 1000);
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Headers", "application/json");
  // 'Access-Control-Allow-Headers':'application/json'
  server.send(200, "application/json", jsonChar);

  ESP.restart();
}
void setup()
{
  EEPROM.begin(1000); // Use 1k for save value
  Serial.begin(9600);
  int r = EEPROM.read(0);
  if (r != 99)
  {
    EEPROM.write(0, 99);
    EEPROM.put(ADDR, wifidata);
    EEPROM.commit();
  }
  else
  {
    EEPROM.get(ADDR, wifidata);
  }
  Serial.println();
  Serial.println("-----------------------------------------------");
  Serial.println(wifidata.ssid);
  Serial.println(wifidata.password);
  Serial.println("-----------------------------------------------");
  WiFiMulti.addAP(wifidata.ssid, wifidata.password);

  // WiFi.mode(WIFI_STA);
  setport();
  // WiFiMulti.addAP("forpi3", "04qwerty");
  WiFiMulti.addAP("forpi", "04qwerty");
  WiFiMulti.addAP("forpi2", "04qwerty");
  // WiFiMulti.addAP("Sirifarm", "0932154741");
  WiFiMulti.addAP("test", "12345678");
  // WiFiMulti.addAP("farm", "12345678");
  WiFiMulti.addAP("pksy", "04qwerty");
  WiFiMulti.addAP("ky_MIFI", "04qwerty");

  int co = 0;
  while (WiFiMulti.run() != WL_CONNECTED) //รอการเชื่อมต่อ
  {
    delay(500);
    setupport();
    Serial.print(".");
    co++;
    if (co > 50)
    {
      setAPMode();
      break;
    }
  }
  if (!apmode)
  {
    checkin();
    ota();
    Serial.println(WiFi.localIP()); // แสดงหมายเลข IP ของ Server
    String mac = WiFi.macAddress();
    Serial.println(mac); // แสดงหมายเลข IP ของ Server
  }
  server.on("/run", run);
  server.on("/ssid", ssid);
  server.on("/status", status);
  server.on("/dht", DHTtoJSON);
  server.on("/ota", trytoota);
  server.on("/setwifi", setwifi);
  server.on("/get", get);
  server.on("/", status);
  server.on("/reset", reset);

  server.on("/setclosetime", setclosetime);
  server.begin(); //เปิด TCP Server
  Serial.println("Server started");
  flipper.attach(1, flip);
  dht.begin();
}
void loop()
{
  server.handleClient();
  // t.update();
  if (checkintime > 600 && counttime < 1)
  {
    checkintime = 0;
    checkin();
  }
  if (otatime > 60 && counttime < 1)
  {
    otatime = 0;
    ota();
  }
  if (porttrick > 0 && counttime > 0)
  {
    porttrick = 0;
    portcheck();
  }

  if (readdhttime > 120 && dhtbuffer.count < 1)
  {
    readdhttime = 0;
    message = "Read DHT";
    readDHT();
  }
}