#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "Apmode.h"
// #include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <Ticker.h>
#include <Wire.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266Ping.h>
#include <LITTLEFS.h>
#include "Configfile.h"

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


#define jsonbuffersize 1024
void loadconfigtoram();
void configdatatofile();
void configwww();
Configfile cfg("/config.cfg");

const String version = "96";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate;
String dayStamp;
String timeStamp;

#define ADDR 100
#define jsonsize 1500

StaticJsonDocument<jsonsize> doc;
// char jsonChar[jsonsize];

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
  int maxconnecttimeout = 30;
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

// ESP8266WebServer server(80);
AsyncWebServer server(80);
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
  configdata.sensorvalue = cfg.getConfig("senservalue", "42.5").toDouble();
  configdata.havedht = cfg.getIntConfig("havedht", 0);
  configdata.havea0 = cfg.getIntConfig("havea0", 0);
  configdata.haveds = cfg.getIntConfig("haveds", 0);
  configdata.havesht = cfg.getIntConfig("havesht", 0);
  configdata.havetorestart = cfg.getIntConfig("havetorestart", 0);
  configdata.maxconnecttimeout = cfg.getIntConfig("maxconnecttimeout",60);
}

void configdatatofile()
{
  // cfg.addConfig("va0", configdata.va0);
  // cfg.addConfig("senservalue", configdata.sensorvalue);
  // cfg.addConfig("havedht", configdata.havedht);
  // cfg.addConfig("havea0", configdata.havea0);
  // cfg.addConfig("haveds", configdata.haveds);
  // cfg.addConfig("havesht", configdata.havesht);
  // cfg.addConfig("haverestart", configdata.havetorestart);
}
void initConfig()
{
  cfg.openFile();
  // cfg.loadConfig();
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
    dhtbuffer.count = 120; // update buffer life time
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
    dhtbuffer.count = 120; // update buffer life time
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
  WiFiClient client;
  Serial.println("CALL " + otahost + " " + updateString);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, otahost, 8080, updateString, version.c_str());
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
// void setp()
// {
//   String configname = server.arg("configname");
//   String value = server.arg("value");

//   cfg.addConfig(configname, value);
//   char buf[500];
//   sprintf(buf, "{\"configname\":\"%s\" , \"value\":\"%s\"}", configname, value);
//   server.send(200, "application/json", buf);
// }
void trytoota()
{
  // String re = "";
  // t_httpUpdate_return ret = ESPhttpUpdate.update("192.168.88.9", 8080, "/espupdate/d1io/",version.c_str());
  // Serial.println(ret);
  // switch (ret)
  // {
  // case HTTP_UPDATE_FAILED:
  //   Serial.println("[update] Update failed.");
  //   re = "Update failed";
  //   break;
  // case HTTP_UPDATE_NO_UPDATES:

  //   Serial.println("[update] Update no Update.");
  //   re = "No update";
  //   break;
  // case HTTP_UPDATE_OK:
  //   Serial.println("[update] Update ok."); // may not called we reboot the ESP
  //   re = "update ok";
  //   break;
  // }
  // doc["name"] = name;
  // doc["ip"] = WiFi.localIP().toString();
  // doc["mac"] = WiFi.macAddress();
  // doc["ssid"] = WiFi.SSID();
  // doc["version"] = version;
  // doc["h"] = pfHum;
  // doc["t"] = pfTemp;
  // doc["uptime"] = uptime;
  // doc["updateresult"] = re;
  // serializeJsonPretty(doc, jsonChar, jsonsize);
  // server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  // server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  // server.sendHeader("Access-Control-Allow-Headers", "application/json");
  // server.send(200, "application/json", jsonChar);
}

// void get()
// {
//   String ssd = server.arg("ssid");
//   String password = server.arg("password");
//   Serial.print("SSD ");
//   Serial.println(ssd);
//   Serial.print("password ");
//   Serial.println(password);
//   cfg.addConfig("ssid", ssd);
//   cfg.addConfig("password", password);
//   loadconfigtoram();
//   // if (ssd != NULL)
//   //   ssd.toCharArray(wifidata.ssid, 50);

//   // if (password != NULL)
//   //   password.toCharArray(wifidata.password, 50);

//   Serial.println("Set ok");
//   // EEPROM.put(ADDR, wifidata);
//   // EEPROM.commit();
//   String re = "<html> <head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><h3>set WIFI TO " + ssd + " <h3><hr><a href='/setconfig'>back</a></html>";
//   server.send(200, "text/html", re);
// }
String makestatus()
{
  char b[jsonbuffersize];
  DynamicJsonDocument dd(jsonbuffersize);
  dd.clear();
  dd["devicetype"] = "d1io";
  dd["name"] = name;
  dd["ip"] = WiFi.localIP().toString();
  dd["mac"] = WiFi.macAddress();
  dd["ssid"] = WiFi.SSID();
  dd["signal"] = WiFi.RSSI();
  dd["version"] = version;
  dd["freemem"] = system_get_free_heap_size();
  dd["heap"] = system_get_free_heap_size();
  dd["h"] = pfHum;
  dd["t"] = pfTemp;
  dd["uptime"] = uptime;
  dd["message"] = message;
  dd["dhtbuffer.time"] = dhtbuffer.count;
  dd["porttrick"] = porttrick;
  dd["counttime"] = counttime;
  dd["savessid"] = wifidata.ssid;
  dd["savepassword"] = wifidata.password;
  dd["otatime"] = otatime;
  dd["D1value"] = digitalRead(D1);
  dd["d1"] = digitalRead(D1);
  dd["D1closetime"] = ports[0].closetime;
  dd["D1delay"] = ports[0].delay;
  dd["D2value"] = digitalRead(D2);
  dd["d2"] = digitalRead(D2);
  dd["D2closetime"] = ports[1].closetime;
  dd["D2delay"] = ports[1].delay;
  dd["D5value"] = digitalRead(D5);
  dd["d5"] = digitalRead(D5);
  dd["D5closetime"] = ports[2].closetime;
  dd["D5delay"] = ports[2].delay;
  dd["D6value"] = digitalRead(D6);
  dd["d6"] = digitalRead(D6);
  dd["D6closetime"] = ports[3].closetime;
  dd["D6delay"] = ports[3].delay;
  dd["D7value"] = digitalRead(D7);
  dd["d7"] = digitalRead(D7);
  dd["D7closetime"] = ports[4].closetime;
  dd["D7delay"] = ports[4].delay;
  dd["D8value"] = digitalRead(D8);
  dd["d8"] = digitalRead(D8);
  dd["D8closetime"] = ports[5].closetime;
  dd["D8delay"] = ports[5].delay;
  dd["config.havetorestart"] = configdata.havetorestart;
  dd["config.restarttime"] = configdata.restarttime;
  dd["config.havedht"] = configdata.havedht;
  dd["restarttime"] = restarttime;
  dd["ntptime"] = timeClient.getFormattedTime();
  dd["ntptimelong"] = timeClient.getEpochTime();
  dd["type"] = type;
  dd["datetime"] = formattedDate;
  dd["date"] = dayStamp;
  dd["time"] = timeStamp;
  dd["load"] = load;
  dd["loadav"] = loadav;
  serializeJson(dd, b, jsonbuffersize);
  return String(b);
}
// void status()
// {
//   server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
//   server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
//   server.sendHeader("Access-Control-Allow-Headers", "application/json");
//   String b = makestatus();
//   server.send(200, "application/json", b);
// }
// void setclosetime()
// {
//   char b[jsonbuffersize];
//   int s = server.arg("time").toInt();
//   digitalWrite(D5, 1);
//   String closetime = server.arg("closetime");
//   ports[2].delay = s;
//   ports[2].value = 1;
//   ports[2].closetime = closetime;
//   DynamicJsonDocument d(jsonbuffersize);
//   d["run"] = "ok";
//   d["countime"] = s;
//   d["closeat"] = closetime;
//   serializeJson(d, b, jsonbuffersize);
//   server.send(200, "application/json", b);
// }

void checkin()
{
  DynamicJsonDocument dy(jsonbuffersize);
  char b[jsonbuffersize];
  WiFiClient client;
  int connectcount = 0;
  if (WiFiMulti.run() != WL_CONNECTED) //รอการเชื่อมต่อ
  {
    return;
  }
  busy = true;
  dy["mac"] = WiFi.macAddress();
  dy["password"] = "";
  dy["ip"] = WiFi.localIP().toString();
  dy["uptime"] = uptime;
  serializeJsonPretty(doc, b, jsonbuffersize);
  // put your main code here, to run repeatedly:
  HTTPClient http; // Declare object of class HTTPClient
  String h = cfg.getConfig("checkinurl", "http://192.168.88.21:3333/checkin");
  //"http://" + hosttraget + "/checkin";
  http.begin(client, h);                              // Specify request destination
  http.addHeader("Content-Type", "application/json"); // Specify content-type header
  http.addHeader("Authorization", "Basic VVNFUl9DTElFTlRfQVBQOnBhc3N3b3Jk");

  int httpCode = http.POST(b);       // Send the request
  String payload = http.getString(); // Get the response payload
  Serial.print(" Http Code:");
  Serial.println(httpCode); // Print HTTP return code
  if (httpCode == 200)
  {
    Serial.print(" Play load:");
    Serial.println(payload); // Print request response payload
    deserializeJson(dy, payload);
    JsonObject obj = dy.as<JsonObject>();
    name = obj["name"].as<String>();
    cfg.addConfig("name", name);
  }

  http.end(); // Close connection
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
/**
 * @brief สำหรับ set port ให้ทำงานตาม value และ delay
 * 
 * @param port  ที่กำหนด
 * @param delay  เวลาการทำงาน
 * @param value logic
 * @param wait หยุดรอ
 */
void addTorun(int port, int delay, int value, int wait)
{
  if (delay > counttime)
    counttime = delay;
  for (int i = 0; i < ioport; i++)
  {
    if (ports[i].port == port)
    {
      ports[i].value = value;

      if (ports[i].delay < delay)
      {
        ports[i].delay = delay;
      }

      ports[i].waittime = wait;
      ports[i].run = 1;
      digitalWrite(ports[i].port, value);
      Serial.println("Set port");
    }
  }
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
// void setconfig()
// {
//   String html = " <!DOCTYPE html> <style> table { font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif; border-collapse: collapse; width: 100%; } td, th { border: 1px solid #ddd; padding: 8px; } tr:nth-child(even) { background-color: #f2f2f2; } tr:hover { background-color: #ddd; } th { padding-top: 12px; padding-bottom: 12px; text-align: left; background-color: #4CAF50; color: white; } button { /* width: 100%; */ background-color: #4CAF50; color: white; padding: 10px 15px; /* margin: 8px 0; */ border: none; border-radius: 4px; cursor: pointer; } .button3 { background-color: #f44336; } </style> <head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <meta charset=\"UTF-8\"> </head> Config in " + String(name) + " version:" + String(version) + " SSID:" + cfg.getConfig("ssid") + " Signel: " + String(WiFi.RSSI()) + " type:" + String(type) + " <table id=\"customres\"> <tr> <td>Parameter</td> <td>value</td> <td>Option</td> </tr> <tr> <td>DHT</td> <td>" + intToEnable(configdata.havedht) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havedht\"> <input type=\"hidden\" name=\"value\" value=\"1\"> <button type=\"submit\" value=\"1\">Enable</button> </form> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havedht\"> <input type=\"hidden\" name=\"value\" value=\"0\"> <button type=\"submit\" class=\"button3\" value=\"1\">Disable</button> </form> </td> </tr> <tr> <td>SHT</td> <td>" + intToEnable(configdata.havesht) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havesht\"> <input type=\"hidden\" name=\"value\" value=\"1\"> <button type=\"submit\" value=\"1\">Enable</button> </form> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havesht\"> <input type=\"hidden\" name=\"value\" value=\"0\"> <button type=\"submit\" class=\"button3\">Disable</button> </form> </td> </tr> <tr> <td>DS</td> <td>" + intToEnable(configdata.haveds) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"haveds\"> <input type=\"hidden\" name=\"value\" value=\"1\"> <button type=\"submit\" value=\"1\">Enable</button> </form> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"haveds\"> <input type=\"hidden\" name=\"value\" value=\"0\"> <button type=\"submit\" class=\"button3\">Disable</button> </form> </td> </tr> <tr> <td>A0</td> <td>" + intToEnable(configdata.havea0) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havea0\"> <input type=\"hidden\" name=\"value\" value=\"1\"> <button type=\"submit\" value=\"1\">Enable</button> </form> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havea0\"> <input type=\"hidden\" name=\"value\" value=\"0\"> <button type=\"submit\" class=\"button3\">Disable</button> </form> </td> </tr> <tr> <td>Have to restart</td> <td>" + intToEnable(configdata.havetorestart) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havetorestart\"> <input type=\"hidden\" name=\"value\" value=\"1\"> <button type=\"submit\" value=\"1\">Enable</button> </form> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"havetorestart\"> <input type=\"hidden\" name=\"value\" value=\"0\"> <button type=\"submit\" class=\"button3\">Disable</button> </form> </td> </tr> <tr> <td>Sensor value </td> <td>" + String(configdata.sensorvalue) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"sensorvalue\"> <input type=\"number\" name=\"value\" value=\"{{valuesesnsor}}\"> <button type=\"submit\" value=\"1\">Save</button> </form> </td> </tr> <tr> <td>Auto restart value </td> <td>" + String(configdata.restarttime) + "</td> <td> <form action=\"/setvalue\"> <input type=\"hidden\" name=\"p\" value=\"restarttime\"> <input type=\"number\" name=\"value\" value=\"{{valuerestarttime}}\"> <button type=\"submit\" value=\"1\">Save</button> </form> </td> </tr> </table> <hr> <form action=\"/get\"> <table> <tr> <td colspan=\"3\">WIFI information</td> </tr> <tr> <td>SSID</td> <td>" + cfg.getConfig("ssid") + "</td> <td><input type=\"text\" name=\"ssid\"></td> </tr> <tr> <td>PASSWORD</td> <td>********</td> <td><input type=\"password\" name=\"password\"></td> </tr> <tr> <td colspan=\"3\" align=\"right\"><button type=\"submit\">Set wifi</button></td> </tr> </table> </form> <form action=\"/restart\"> <button type=\"submit\" class=\"button3\" value=\"Restart\">Restart</button> </form> ky@pixka.me 2020 </html>";
//   server.send(200, "text/html", html);
// }
// void setvalue()
// {
//   String v = server.arg("p");
//   String value = server.arg("value");
//   String value2 = server.arg("value2");
//   Serial.print("Set config:");
//   Serial.print(v);
//   Serial.printf("to %s", value.c_str());
//   if (v.equals("va0"))
//   {
//     configdata.va0 = value.toFloat();
//     cfg.addConfig("va0", configdata.va0);
//   }
//   else if (v.equals("sensorvalue"))
//   {
//     configdata.sensorvalue = value.toFloat();
//     cfg.addConfig("senservalue", configdata.sensorvalue);
//   }
//   else if (v.equals("havedht"))
//   {
//     configdata.havedht = value.toInt();
//     cfg.addConfig("havedht", configdata.havedht);
//   }
//   else if (v.equals("haveds"))
//   {
//     configdata.haveds = value.toInt();
//     cfg.addConfig("haveds", configdata.haveds);
//   }
//   else if (v.equals("havea0"))
//   {
//     configdata.havea0 = value.toInt();
//     cfg.addConfig("havea0", configdata.havea0);
//   }
//   else if (v.equals("havetorestart"))
//   {
//     configdata.havetorestart = value.toInt();
//     cfg.addConfig("haverestart", configdata.havetorestart);
//   }
//   else if (v.equals("havesht"))
//   {
//     configdata.havesht = value.toInt();
//     cfg.addConfig("havesht", configdata.havesht);
//   }
//   else if (v.equals("restarttime"))
//   {
//     configdata.restarttime = value.toInt();
//     cfg.addConfig("restarttime", configdata.restarttime);
//   }

//   // configdatatofile();

//   String re = "<html> <head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><h3>set " + v + " TO " + value + " <h3><hr><a href='/setconfig'>back</a> <script type=\"text/JavaScript\"> redirectTime = \"1500\"; redirectURL = \"/setconfig\"; function timedRedirect() { setTimeout(\"location.href = redirectURL;\",redirectTime); } </script></html>";
//   server.send(200, "text/html", re);
// }

// void configfile()
// {
//   char b[jsonbuffersize];
//   DynamicJsonDocument configbuf = cfg.getAll();
//   serializeJsonPretty(configbuf, b, jsonbuffersize);
//   server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
//   server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
//   server.sendHeader("Access-Control-Allow-Headers", "application/json");
//   Serial.println(b);
//   server.send(200, "application/json", b);
//   Serial.println("get all config file");
// }
// void setConfigfile()
// {
//   String v = server.arg("configname");
//   String value = server.arg("value");
//   cfg.addConfig(v, value);
//   server.send(200, "application/json", "{\"setconfig\":\"" + v + "\",\"value\":\"" + value + "\"}");
// }
void setHttp()
{

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
             String re =  makestatus();
              request->send(200, "application/json", re); });
  server.on("/run", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
                Serial.println("Run");
  String p = request->arg("port");
  char b[jsonbuffersize];
  DynamicJsonDocument dy(jsonbuffersize);
  if (p.equals("test"))
  {
    message = "test port ";
    canuseled = 0;
    // doc.clear();
    dy["status"] = "ok";
    dy["port"] = p;
    dy["mac"] = WiFi.macAddress();
    dy["ip"] = WiFi.localIP().toString();
    dy["name"] = name;
    dy["uptime"] = uptime;
    dy["ntptime"] = timeClient.getFormattedTime();
    dy["ntptimelong"] = timeClient.getEpochTime();

    serializeJson(dy, b, jsonbuffersize);
    request->send(200, "application/json", b);
    for (int i = 0; i < 40; i++)
    {
      digitalWrite(2, !digitalRead(2));
      delay(200);
    }
    canuseled = 1;
    return;
  }
  String v =  request->arg("value");
  String d =  request->arg("delay");
  String w =  request->arg("wait");
  Serial.println("Port: " + p + " value : " + v + " delay: " + d);
  message = String("run port ") + String(p) + String(" value") + String(" delay ") + String(d) + " " + timeClient.getFormattedDate();
  int value = v.toInt();
  int port = getPort(p);
  if (p == NULL)
  {
    port = D5;
    v = "1";
  }
  addTorun(port, d.toInt(), v.toInt(), w.toInt());
  dy["status"] = "ok";
  dy["port"] = p;
  dy["runtime"] = d;
  dy["value"] = value;
  dy["mac"] = WiFi.macAddress();
  dy["ip"] = WiFi.localIP().toString();
  dy["uptime"] = uptime;
  serializeJson(dy, b, jsonbuffersize);
   request->send(200, "application/json", b); });
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
            char b[jsonbuffersize];
  DynamicJsonDocument configbuf = cfg.getAll();
  serializeJson(configbuf, b, jsonbuffersize);
  Serial.println(b);
  Serial.println("get all config file");
              request->send(200, "application/json", b); });
  server.on("/setconfig", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
         String v = request->arg("configname");
  String value = request->arg("value");
  cfg.addConfig(v, value);
  request->send(200, "application/json", "{\"setconfig\":\"" + v + "\",\"value\":\"" + value + "\"}"); });
  // server.send(200, "application/json", b);

  // server.on("/run", run);
  // server.on("/ssid", ssid);
  // server.on("/status", status);
  // server.on("/dht", DHTtoJSON);
  // server.on("/ota", trytoota);
  // server.on("/update", trytoota);
  // server.on("/setwifi", setwifi);
  // server.on("/get", get);
  // server.on("/setconfig", setconfig);
  // server.on("/configfile", configfile);
  // server.on("/setconfigfile", setConfigfile);
  // server.on("/config", configwww);
  // server.on("/setp", setp);
  // server.on("/", status);
  // server.on("/reset", reset);
  // server.on("/restart", reset);
  // server.on("/setvalue", setvalue);
  // server.on("/setclosetime", setclosetime);
  // server.on("/resettodefault", resettodefault);
  server.begin(); //เปิด TCP Server
  Serial.println("Server started");
}
void Apmoderun()
{
  ApMode ap("cfg.cfg");
   ap.setApname("ESP Sensor AP Mode");
   ap.run();
}
void wificonnect()
{

    WiFi.mode(WIFI_STA);
    Serial.println();
    Serial.println("-----------------------------------------------");
    Serial.println(cfg.getConfig("ssid", "forpi"));
    Serial.println(cfg.getConfig("password", "04qwerty"));
    Serial.println("-----------------------------------------------");
   
    WiFi.begin(cfg.getConfig("ssid", "forpi").c_str(), cfg.getConfig("password", "04qwerty").c_str());
    Serial.print("connect.");
    int ft = 0;
    // display.clear();
    while (WiFi.status() != WL_CONNECTED) //รอการเชื่อมต่อ
    {
        delay(250);
       
       
        ft++;
        if (ft > configdata.maxconnecttimeout)
        {
            Serial.println("Connect main wifi timeout");
            apmode = 1;
            break;
        }
        Serial.print(".");
    }

    if (apmode)
    {
        Apmoderun();
    }
    else
    {

        Serial.println(WiFi.localIP()); // แสดงหมายเลข IP ของ Server
        String ip = WiFi.localIP().toString();
        String mac = WiFi.macAddress();
        Serial.println(mac); // แสดงหมายเลข IP ของ Server

    }
}
void setup()
{
  Serial.begin(9600);
  cfg.setbuffer(jsonbuffersize);
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

  wificonnect();
  setHttp();
  flipper.attach(1, flip);
  dht.begin();
  timeClient.begin();
  timeClient.setTimeOffset(25200); // Thailand +7 = 25200
}
// void configwww()
// {

//   DynamicJsonDocument d = cfg.getAll();
//   int size = d.capacity();
//   char buf[size];
//   serializeJsonPretty(d, buf, size);
//   Serial.print("Size:");
//   Serial.println(size);
//   server.send(200, "application/json", buf);
// }
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
  // long s = millis();
  // server.handleClient();
  // t.update();
  if (checkintime > cfg.getIntConfig("checkintime", 600) && counttime < 1)
  {
    checkintime = 0;
    checkin();
  }
  if (otatime > cfg.getIntConfig("otatime", 1500) && counttime < 1)
  {
    updateNTP();
    if (WiFiMulti.run() == WL_CONNECTED)
    {
      WiFi.softAPdisconnect(true);
    }
    otatime = 0;
    ota();
  }
  if (porttrick > 0 && counttime >= 0)
  {
    porttrick = 0;
    portcheck();
  }

  if (configdata.havedht && readdhttime > cfg.getIntConfig("readdhttime", 120) && dhtbuffer.count < 1)
  {
    readdhttime = 0;
    message = "Read DHT";
    readDHT();
  }

  if (configdata.havetorestart && restarttime > cfg.getIntConfig("checkconnection", 600))
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

  if (apmodetimeout > cfg.getIntConfig("reconnecttime", 300))
  {
    WiFi.reconnect();
  }
  // if (makestatuscount > 2)
  // {
  //   makestatus();
  //   makestatuscount = 0;
  // }
  // load = millis() - s;
  // loadcount++;
  // loadtotal += load;
  // loadav = loadtotal / loadcount;
}