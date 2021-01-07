#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <Ticker.h>
#include <Wire.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266Ping.h>
#include "Configfile.h"
void loadconfigtoram();
void configdatatofile();
Configfile cfg("/config.cfg");

const String version = "91";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate;
String dayStamp;
String timeStamp;

#define ADDR 100
#define jsonsize 1500

StaticJsonDocument<jsonsize> doc;
char jsonChar[jsonsize];

int makestatuscount = 0;
long uptime = 0;
long otatime = 0;
long checkintime = 0;
long timetoreaddht = 0;
long porttrick = 0;
long readdhttime = 0;
int apmode = 0;
int restarttime = 0;
int apmodetimeout = 0;
int canuseled = 1;
long load = 0;
long loadcount = 0;
double loadav = 0;
double loadtotal = 0;
#define ioport 7
String name = "d1io";
const String type = "D1IO";

extern "C"
{
#include "user_interface.h"
}
long counttime = 0;
String hosttraget = "point.pixka.me:2222";
String otahost = "point.pixka.me";
String updateString = "/espupdate/d1io/" + version;
String message = "";
struct
{
  int readtmpvalue = 120;
  int a0readtime = 120;
  float va0 = 0.5;
  float sensorvalue = 42.5;
  boolean havedht = false;
  boolean haveds = false;
  boolean havea0 = false;
  boolean havetorestart = false; //สำหรับบอกว่าถ้าติดต่อ wifi ไม่ได้ให้ restart
  boolean havesht = false;
  int restarttime = 360; // หน่วยเป็นวิ
} configdata;
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
void loadconfigtoram()
{
  configdata.va0 = cfg.getConfig("va0").toFloat();
  configdata.sensorvalue = cfg.getConfig("senservalue").toDouble();

  configdata.havedht = cfg.getIntConfig("havedht");
  configdata.havea0 = cfg.getIntConfig("havea0");
  configdata.haveds = cfg.getIntConfig("haveds");
  configdata.havesht = cfg.getIntConfig("havesht");
  configdata.havetorestart = cfg.getIntConfig("havetorestart");
}

void configdatatofile()
{
  cfg.addConfig("va0", configdata.va0);
  cfg.addConfig("senservalue", configdata.sensorvalue);
  cfg.addConfig("havedht", configdata.havedht);
  cfg.addConfig("havea0", configdata.havea0);
  cfg.addConfig("haveds", configdata.haveds);
  cfg.addConfig("havesht", configdata.havesht);
  cfg.addConfig("haverestart", configdata.havetorestart);
}
void initConfig()
{
  cfg.openFile();
  cfg.loadConfig();
  Serial.printf("\n***** Init config **** \n");
  delay(1000);

  cfg.addConfig("ssid", "forpi");
  cfg.addConfig("password", "04qwerty");

  cfg.addConfig("va0", 0.5);
  cfg.addConfig("sensorvalue", 42.5);

  cfg.addConfig("D5mode", OUTPUT);
  cfg.addConfig("D5initvalue", 0);
  cfg.addConfig("D6mode", OUTPUT);
  cfg.addConfig("D6initvalue", 0);
  cfg.addConfig("D7mode", OUTPUT);
  cfg.addConfig("D7initvalue", 0);
  cfg.addConfig("D8mode", OUTPUT);
  cfg.addConfig("D8initvalue", 0);

  cfg.addConfig("havedht", 0);
  cfg.addConfig("havea0", 0);
  cfg.addConfig("haveds", 0);
  cfg.addConfig("havertc", 0);
  cfg.addConfig("havertc", 0);
  cfg.addConfig("havepmsensor", 0);
  cfg.addConfig("havesht", 0);
  cfg.addConfig("havesonic", 0);
  cfg.addConfig("haveoled", 0);
}
Portio ports[ioport];
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
  WiFi.softAP("ESP-IO" + mac);
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
}
void updateNTP()
{
  timeClient.update();
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  Serial.print("HOUR: ");
  Serial.println(timeStamp);
}
void ota()
{
  Serial.println("CALL " + otahost + " " + updateString);
  t_httpUpdate_return ret = ESPhttpUpdate.update(otahost, 8080, updateString, version);
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

void trytoota()
{
  String re = "";
  t_httpUpdate_return ret = ESPhttpUpdate.update("192.168.88.9", 8080, "/espupdate/d1io/1");
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
  doc["name"] = name;
  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["ssid"] = WiFi.SSID();
  doc["version"] = version;
  doc["h"] = pfHum;
  doc["t"] = pfTemp;
  doc["uptime"] = uptime;
  doc["updateresult"] = re;
  serializeJsonPretty(doc, jsonChar, jsonsize);
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Headers", "application/json");
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
  String re = "<html> <head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><h3>set WIFI TO " + ssd + " <h3><hr><a href='/setconfig'>back</a></html>";
  server.send(200, "text/html", re);
}
void makestatus()
{
  doc.clear();
  doc["name"] = name;
  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["ssid"] = WiFi.SSID();
  doc["signal"] = WiFi.RSSI();
  doc["version"] = version;
  doc["freemem"] = system_get_free_heap_size();
  doc["heap"] = system_get_free_heap_size();
  doc["h"] = pfHum;
  doc["t"] = pfTemp;
  doc["uptime"] = uptime;
  doc["message"] = message;
  doc["dhtbuffer.time"] = dhtbuffer.count;
  doc["porttrick"] = porttrick;
  doc["counttime"] = counttime;
  doc["savessid"] = wifidata.ssid;
  doc["savepassword"] = wifidata.password;
  doc["otatime"] = otatime;
  doc["D1value"] = digitalRead(D1);
  doc["d1"] = digitalRead(D1);
  doc["D1closetime"] = ports[0].closetime;
  doc["D1delay"] = ports[0].delay;
  doc["D2value"] = digitalRead(D2);
  doc["d2"] = digitalRead(D2);
  doc["D2closetime"] = ports[1].closetime;
  doc["D2delay"] = ports[1].delay;
  doc["D5value"] = digitalRead(D5);
  doc["d5"] = digitalRead(D5);
  doc["D5closetime"] = ports[2].closetime;
  doc["D5delay"] = ports[2].delay;
  doc["D6value"] = digitalRead(D6);
  doc["d6"] = digitalRead(D6);
  doc["D6closetime"] = ports[3].closetime;
  doc["D6delay"] = ports[3].delay;
  doc["D7value"] = digitalRead(D7);
  doc["d7"] = digitalRead(D7);
  doc["D7closetime"] = ports[4].closetime;
  doc["D7delay"] = ports[4].delay;
  doc["D8value"] = digitalRead(D8);
  doc["d8"] = digitalRead(D8);
  doc["D8closetime"] = ports[5].closetime;
  doc["D8delay"] = ports[5].delay;
  doc["config.havetorestart"] = configdata.havetorestart;
  doc["config.restarttime"] = configdata.restarttime;
  doc["config.havedht"] = configdata.havedht;
  doc["restarttime"] = restarttime;
  doc["ntptime"] = timeClient.getFormattedTime();
  doc["ntptimelong"] = timeClient.getEpochTime();
  doc["type"] = type;
  doc["datetime"] = formattedDate;
  doc["date"] = dayStamp;
  doc["time"] = timeStamp;
  doc["load"] = load;
  doc["loadav"] = loadav;
  serializeJsonPretty(doc, jsonChar, jsonsize);
}
void status()
{
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Headers", "application/json");
  server.send(200, "application/json", jsonChar);
}
void setclosetime()
{
  int s = server.arg("time").toInt();
  digitalWrite(D5, 1);
  String closetime = server.arg("closetime");
  ports[2].delay = s;
  ports[2].value = 1;
  ports[2].closetime = closetime;
  doc["run"] = "ok";
  doc["countime"] = s;
  doc["closeat"] = closetime;
  serializeJsonPretty(doc, jsonChar, jsonsize);
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
  doc["mac"] = WiFi.macAddress();
  doc["password"] = "";
  doc["ip"] = WiFi.localIP().toString();
  doc["uptime"] = uptime;
  serializeJsonPretty(doc, jsonChar, jsonsize);
  // put your main code here, to run repeatedly:
  HTTPClient http; //Declare object of class HTTPClient
  String h = "http://" + hosttraget + "/checkin";
  http.begin(h);                                      //Specify request destination
  http.addHeader("Content-Type", "application/json"); //Specify content-type header
  http.addHeader("Authorization", "Basic VVNFUl9DTElFTlRfQVBQOnBhc3N3b3Jk");

  int httpCode = http.POST(jsonChar); //Send the request
  String payload = http.getString();  //Get the response payload
  Serial.print(" Http Code:");
  Serial.println(httpCode); //Print HTTP return code
  if (httpCode == 200)
  {
    Serial.print(" Play load:");
    Serial.println(payload); //Print request response payload
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
  return 0;
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
  // doc.clear();
  doc["t"] = pfTemp;
  doc["h"] = pfHum;
  doc["ip"] = WiFi.macAddress();
  serializeJsonPretty(doc, jsonChar, jsonsize);
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

  if (p.equals("test"))
  {
    message = "test port ";
    canuseled = 0;
    // doc.clear();
    doc["status"] = "ok";
    doc["port"] = p;
    doc["mac"] = WiFi.macAddress();
    doc["ip"] = WiFi.localIP().toString();
    doc["name"] = name;
    doc["uptime"] = uptime;
    doc["ntptime"] = timeClient.getFormattedTime();
    doc["ntptimelong"] = timeClient.getEpochTime();

    serializeJsonPretty(doc, jsonChar, jsonsize);
    server.send(200, "application/json", jsonChar);
    for (int i = 0; i < 40; i++)
    {
      digitalWrite(2, !digitalRead(2));
      delay(200);
    }
    canuseled = 1;

    return;
  }
  String v = server.arg("value");
  String d = server.arg("delay");
  String w = server.arg("wait");
  Serial.println("Port: " + p + " value : " + v + " delay: " + d);
  message = String("run port ") + String(p) + String(" value") + String(" delay ") + String(d) + " " + timeClient.getFormattedDate();
  int value = v.toInt();

  int port = getPort(p);
  addTorun(port, d.toInt(), v.toInt(), w.toInt());
  doc["status"] = "ok";
  doc["port"] = p;
  doc["runtime"] = d;
  doc["value"] = value;
  doc["mac"] = WiFi.macAddress();
  doc["ip"] = WiFi.localIP().toString();
  doc["uptime"] = uptime;
  serializeJsonPretty(doc, jsonChar, jsonsize);
  server.send(200, "application/json", jsonChar);
}
void setwifi()
{
  server.send(200, "text/html", index_html);
}
void ssid()
{
  doc["ssid"] = WiFi.SSID();
  serializeJsonPretty(doc, jsonChar, jsonsize);
  server.send(200, "applic ation/json", jsonChar);
}
void flip()
{
  makestatuscount++;
  uptime++;      //เวลา run
  otatime++;     //เพิ่มการ update
  checkintime++; //เพิ่มการนับ
  porttrick++;   //บอกว่า 1 วิละ
  readdhttime++; //บอกเวลา สำหรับอ่าน DHT
  restarttime++;
  if (apmode)
    apmodetimeout++;
  if (counttime > 0)
    counttime--;

  // get the current state of GPIO1 pin
  if (canuseled)
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
String intToEnable(int i)
{
  if (i == 0)
  {
    return String("Disable");
  }
  else
  {
    return String("Enable");
  }
}
void setconfig()
{
  String html = " <!DOCTYPE html> <style> table { font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif; border-collapse: collapse; width: 100%; } td, th { border: 1px solid #ddd; padding: 8px; } tr:nth-child(even) { background-color: #f2f2f2; } tr:hover { background-color: #ddd; } th { padding-top: 12px; padding-bottom: 12px; text-align: left; background-color: #4CAF50; color: white; } button { /* width: 100%; */ background-color: #4CAF50; color: white; padding: 10px 15px; /* margin: 8px 0; */ border: none; border-radius: 4px; cursor: pointer; } .button3 { background-color: #f44336; } </style> <head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <meta charset=\"UTF-8\"> </head> Config in " + String(name) + " version:" + String(version) + " SSID:" + WiFi.SSID() + " Signel: " + String(WiFi.RSSI()) + " type:" + String(type) + " <table id=\"customres\"> <tr> <td>Parameter</td> <td>value</td> <td>Option</td> </tr> <tr> <td>DHT</td> <td>" + intToEnable(configdata.havedht) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havedht\"> <input type=\"hidden\" name=\"value\" value=\"1\"> <button type=\"submit\" value=\"1\">Enable</button> </form> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havedht\"> <input type=\"hidden\" name=\"value\" value=\"0\"> <button type=\"submit\" class=\"button3\" value=\"1\">Disable</button> </form> </td> </tr> <tr> <td>SHT</td> <td>" + intToEnable(configdata.havesht) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havesht\"> <input type=\"hidden\" name=\"value\" value=\"1\"> <button type=\"submit\" value=\"1\">Enable</button> </form> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havesht\"> <input type=\"hidden\" name=\"value\" value=\"0\"> <button type=\"submit\" class=\"button3\">Disable</button> </form> </td> </tr> <tr> <td>DS</td> <td>" + intToEnable(configdata.haveds) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"haveds\"> <input type=\"hidden\" name=\"value\" value=\"1\"> <button type=\"submit\" value=\"1\">Enable</button> </form> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"haveds\"> <input type=\"hidden\" name=\"value\" value=\"0\"> <button type=\"submit\" class=\"button3\">Disable</button> </form> </td> </tr> <tr> <td>A0</td> <td>" + intToEnable(configdata.havea0) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havea0\"> <input type=\"hidden\" name=\"value\" value=\"1\"> <button type=\"submit\" value=\"1\">Enable</button> </form> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havea0\"> <input type=\"hidden\" name=\"value\" value=\"0\"> <button type=\"submit\" class=\"button3\">Disable</button> </form> </td> </tr> <tr> <td>Have to restart</td> <td>" + intToEnable(configdata.havetorestart) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havetorestart\"> <input type=\"hidden\" name=\"value\" value=\"1\"> <button type=\"submit\" value=\"1\">Enable</button> </form> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havetorestart\"> <input type=\"hidden\" name=\"value\" value=\"0\"> <button type=\"submit\" class=\"button3\">Disable</button> </form> </td> </tr> <tr> <td>Sensor value </td> <td>" + String(configdata.sensorvalue) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"sensorvalue\"> <input type=\"number\" name=\"value\" value=\"{{valuesesnsor}}\"> <button type=\"submit\" value=\"1\">Save</button> </form> </td> </tr> <tr> <td>Auto restart value </td> <td>" + String(configdata.restarttime) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"restarttime\"> <input type=\"number\" name=\"value\" value=\"{{valuerestarttime}}\"> <button type=\"submit\" value=\"1\">Save</button> </form> </td> </tr> </table> <hr> <form action=\"/get\"> <table> <tr> <td colspan=\"3\">WIFI information</td> </tr> <tr> <td>SSID</td> <td>" + String(wifidata.ssid) + "</td> <td><input type=\"text\" name=\"ssid\"></td> </tr> <tr> <td>PASSWORD</td> <td>********</td> <td><input type=\"password\" name=\"password\"></td> </tr> <tr> <td colspan=\"3\" align=\"right\"><button type=\"submit\">Set wifi</button></td> </tr> </table> </form> <form action=\"/restart\"> <button type=\"submit\" class=\"button3\" value=\"Restart\">Restart</button> </form> ky@pixka.me 2020 </html>";
  server.send(200, "text/html", html);
}
void setvalue()
{
  String v = server.arg("p");
  String value = server.arg("value");
  String value2 = server.arg("value2");
  if (v.equals("va0"))
  {
    configdata.va0 = value.toFloat();
  }
  else if (v.equals("sensorvalue"))
  {
    configdata.sensorvalue = value.toFloat();
  }
  else if (v.equals("havedht"))
  {
    configdata.havedht = value.toInt();
  }
  else if (v.equals("haveds"))
  {
    configdata.haveds = value.toInt();
  }
  else if (v.equals("havea0"))
  {
    configdata.havea0 = value.toInt();
  }
  else if (v.equals("havetorestart"))
  {
    configdata.havetorestart = value.toInt();
  }
  else if (v.equals("havesht"))
  {
    configdata.havesht = value.toInt();
  }
  else if (v.equals("restarttime"))
  {
    configdata.restarttime = value.toInt();
  }

  configdatatofile();

  String re = "<html> <head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><h3>set " + v + " TO " + value + " <h3><hr><a href='/setconfig'>back</a> <script type=\"text/JavaScript\"> redirectTime = \"1500\"; redirectURL = \"/setconfig\"; function timedRedirect() { setTimeout(\"location.href = redirectURL;\",redirectTime); } </script></html>";
  server.send(200, "text/html", re);
}
void reset()
{
  doc.clear();
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

  // char jsonChar[jsonsize];
  serializeJsonPretty(doc, jsonChar, jsonsize);
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Headers", "application/json");
  // 'Access-Control-Allow-Headers':'application/json'
  server.send(200, "application/json", jsonChar);
  ESP.restart();
}
void resettodefault()
{
  cfg.resettodefault();
  server.send(200, "text/html", "Reset config file ok");
  delay(1000);
  ESP.restart();
}
void setHttp()
{
  server.on("/run", run);
  server.on("/ssid", ssid);
  server.on("/status", status);
  server.on("/dht", DHTtoJSON);
  server.on("/ota", trytoota);
  server.on("/update", trytoota);
  server.on("/setwifi", setwifi);
  server.on("/get", get);
  server.on("/setconfig", setconfig);
  server.on("/", status);
  server.on("/reset", reset);
  server.on("/restart", reset);
  server.on("/setvalue", setvalue);
  server.on("/setclosetime", setclosetime);
  server.on("/resettodefault", resettodefault);
  server.begin(); //เปิด TCP Server
  Serial.println("Server started");
}
void setup()
{
  Serial.begin(9600);

  if (!cfg.openFile())
  {
    initConfig();
  }
  loadconfigtoram();
  setport();
  Serial.println();
  Serial.println("-----------------------------------------------");
  Serial.println(cfg.getConfig("ssid"));
  Serial.println(cfg.getConfig("password"));
  Serial.println("-----------------------------------------------");
  WiFiMulti.addAP(cfg.getConfig("ssid", "forpi").c_str(), cfg.getConfig("password", "04qwerty").c_str());
  int ft = 0;
  while (WiFiMulti.run() != WL_CONNECTED) //รอการเชื่อมต่อ
  {

    delay(500);
    Serial.print("#");
    ft++;
    if (ft > 30)
    {
      Serial.println("Connect main wifi timeout");
      apmode = 1;
      break;
    }
  }
  WiFiMulti.addAP("forpi", "04qwerty");
  WiFiMulti.addAP("forpi2", "04qwerty");
  WiFiMulti.addAP("forpi5", "04qwerty");
  WiFiMulti.addAP("forpi4", "04qwerty");
  WiFiMulti.addAP("Sirifarm", "0932154741");
  WiFiMulti.addAP("test", "12345678");
  WiFiMulti.addAP("pksy", "04qwerty");
  WiFiMulti.addAP("ky_MIFI", "04qwerty");
  setupport();
  int co = 0;
  while (WiFiMulti.run() != WL_CONNECTED) //รอการเชื่อมต่อ
  {
    delay(500);

    Serial.print(".");
    co++;
    if (co > 50)
    {
      setAPMode();
      break;
    }
  }

  if (WiFiMulti.run() == WL_CONNECTED)
  {
    apmode = 0;
    message = String(" connect to :") + WiFi.SSID();
  }
  if (!apmode)
  {
    ota();
    checkin();
    WiFi.softAPdisconnect(true);
    Serial.println(WiFi.localIP()); // แสดงหมายเลข IP ของ Server
    String mac = WiFi.macAddress();
    Serial.println(mac); // แสดงหมายเลข IP ของ Server
  }
  setHttp();
  flipper.attach(1, flip);
  dht.begin();
  timeClient.begin();
  timeClient.setTimeOffset(25200); //Thailand +7 = 25200
}
void printIPAddressOfHost(const char *host)
{
  IPAddress resolvedIP;
  if (!WiFi.hostByName(host, resolvedIP))
  {
    Serial.println("DNS lookup failed.  Count..." + String(configdata.restarttime));
    Serial.flush();
    // restarttime++; //เพิ่มขึ้น
    if (restarttime > configdata.restarttime && configdata.havetorestart)
      ESP.reset();
  }
  restarttime = 0; //ติดต่อได้ก็ reset ไปเลย
  Serial.print(host);
  Serial.print(" IP: ");
  Serial.println(resolvedIP);
}
void loop()
{
  long s = millis();
  server.handleClient();
  // t.update();
  if (checkintime > 600 && counttime < 1)
  {
    checkintime = 0;
    checkin();
  }
  if (otatime > 60 && counttime < 1)
  {

    updateNTP();
    if (WiFiMulti.run() == WL_CONNECTED)
    {
      WiFi.softAPdisconnect(true);
    }
    otatime = 0;
    // timeClient.forceUpdate();

    ota();
  }
  if (porttrick > 0 && counttime >= 0)
  {
    porttrick = 0;
    portcheck();
  }

  if (configdata.havedht && readdhttime > 120 && dhtbuffer.count < 1)
  {
    readdhttime = 0;
    message = "Read DHT";
    readDHT();
  }

  if (configdata.havetorestart && restarttime > 60)
  { //ใช้สำหรับ check ว่า ยังติดต่อ server ได้เปล่าถ้าได้ก็ผ่านไป
    // printIPAddressOfHost("fw.pixka.me");
    if (!Ping.ping(WiFi.gatewayIP()))
    {
      WiFi.disconnect();
      delay(1000);
      WiFi.reconnect();
    }
    restarttime = 0;
  }

  if (apmodetimeout > 600)
  {
    WiFi.reconnect();
  }
  if (makestatuscount > 2)
  {
    makestatus();
    makestatuscount = 0;
  }
  load = millis() - s;
  loadcount++;
  loadtotal += load;
  loadav = loadtotal / loadcount;
}