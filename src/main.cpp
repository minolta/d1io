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
#define jsonsize 1200
StaticJsonDocument<jsonsize> doc;
// #include "SSD1306.h"
long uptime = 0;
long otatime = 0;
long checkintime = 0;
long timetoreaddht = 0;
long porttrick = 0;
long readdhttime = 0;
int apmode = 0;
int restarttime = 0;
// #include <ESP8266Ping.h>
// #define useI2C 1
#define ioport 7
String name = "d1io";
String type = "D1IO";
String version = "67";
extern "C"
{
#include "user_interface.h"
}
long counttime = 0;
// SSD1306 display(0x3C, D2, D1);
//D2 = SDA  D1 = SCL
// String hosttraget = "192.168.88.9:2222";
String hosttraget = "pi3.pixka.me:2222";
// String otahost = "192.168.88.9";
String otahost = "pi3.pixka.me";
String updateString = "/espupdate/d1io/" + version;
// SSD1306 display(0x3C, RX, TX);
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
const char setupconfig[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<style>
    input[type=text],
    select {
        width: 100%;
        padding: 12px 20px;
        margin: 8px 0;
        display: inline-block;
        border: 1px solid #ccc;
        border-radius: 4px;
        box-sizing: border-box;
    }
    input[type=password] {
  width: 100%;
  padding: 12px 20px;
  margin: 8px 0;
  display: inline-block;
  border: 1px solid #ccc;
  border-radius: 4px;
  box-sizing: border-box;
}
    input[type=submit] {
        width: 100%;
        background-color: #4CAF50;
        color: white;
        padding: 14px 20px;
        margin: 8px 0;
        border: none;
        border-radius: 4px;
        cursor: pointer;
    }
    
    input[type=submit]:hover {
        background-color: #45a049;
    }
    
    div {
        border-radius: 5px;
        background-color: #f2f2f2;
        padding: 20px;
    }
</style>
<script>
</script>

<head>
    <title>ESP WIFI Config</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>

<body>

<div>
<h3>Set config</h3>
    <form action="/setvalue">
       Have<select id="cars" name="p">
            <option value="havedht">Dht</option>
            <option value="havesht">SHT</option>
            <option value="haveds">DS</option>
            <option value="havea0">A0</option>
            <option value="havetorestart">Auto restart</option>
          </select>

        Set to<select name="value">
            <option value="1">Enable</option>
            <option value="0">Disable</option>
          </select>
        <input type="submit" value="Set">
    </form>
    <form action="/restart">
        <input type="submit" value="Restart">
    </form>


    <form action="/get">
    SSID: <input type="text" name="ssid">
    PASSWORD: <input type="password" name="password">
    <input type="submit" value="setwifi">
  </form>



   <form action="/setvalue">
       Set value : <select id="cars" name="p">
            <option value="sensorvalue">Sensor value</option>
            <option value="restarttime">Restarttime value</option>
            
          </select>

        Set to <input type="text" name="value">
        <input type="submit" value="Set">
    </form>
    </div>
    <br> contract ky@pixka.me
</body>

</html>
)rawliteral";
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
  doc.clear();

  doc["name"] = name;
  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["ssid"] = WiFi.SSID();
  doc["version"] = version;
  doc["h"] = pfHum;
  doc["t"] = pfTemp;
  doc["uptime"] = uptime;
  doc["updateresult"] = re;
  char jsonChar[jsonsize];
  serializeJsonPretty(doc, jsonChar, jsonsize);
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
  doc.clear();
  doc["name"] = name;
  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["ssid"] = WiFi.SSID();
  doc["signal"] = WiFi.RSSI();
  doc["version"] = version;
  doc["freemem"] = system_get_free_heap_size();
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
  doc["D1closetime"] = ports[0].closetime;
  doc["D1delay"] = ports[0].delay;
  doc["D2value"] = digitalRead(D2);
  doc["D2closetime"] = ports[1].closetime;
  doc["D2delay"] = ports[1].delay;
  doc["D5value"] = digitalRead(D5);
  doc["D5closetime"] = ports[2].closetime;
  doc["D5delay"] = ports[2].delay;
  doc["D6value"] = digitalRead(D6);
  doc["D6closetime"] = ports[3].closetime;
  doc["D6delay"] = ports[3].delay;
  doc["D7value"] = digitalRead(D7);
  doc["D7closetime"] = ports[4].closetime;
  doc["D7delay"] = ports[4].delay;
  doc["D8value"] = digitalRead(D8);
  doc["D8closetime"] = ports[5].closetime;
  doc["D8delay"] = ports[5].delay;
  doc["config.havetorestart"] = configdata.havetorestart;
  doc["config.restarttime"] = configdata.restarttime;
  doc["config.havedht"] = configdata.havedht;
  doc["restarttime"] = restarttime;
  char jsonChar[jsonsize];
  serializeJsonPretty(doc, jsonChar, jsonsize);
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Headers", "application/json");
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
  doc.clear();
  doc["run"] = "ok";
  doc["countime"] = s;
  doc["closeat"] = closetime;
  char jsonChar[jsonsize];
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
  // StaticJsonDocument<500> doc;
  doc.clear();
  doc["mac"] = WiFi.macAddress();
  doc["password"] = "";
  doc["ip"] = WiFi.localIP().toString();
  doc["uptime"] = uptime;
  char JSONmessageBuffer[jsonsize];
  serializeJsonPretty(doc, JSONmessageBuffer, jsonsize);
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
    // DynamicJsonDocument doc(1024);
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
  doc.clear();
  doc["t"] = pfTemp;
  doc["h"] = pfHum;
  doc["ip"] = WiFi.macAddress();
  char jsonChar[jsonsize];
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
  String v = server.arg("value");
  String d = server.arg("delay");
  String w = server.arg("wait");
  Serial.println("Port: " + p + " value : " + v + " delay: " + d);
  int value = v.toInt();

  int port = getPort(p);
  addTorun(port, d.toInt(), v.toInt(), w.toInt());
  doc.clear();
  doc["status"] = "ok";
  doc["port"] = p;
  doc["runtime"] = d;
  doc["value"] = value;
  doc["mac"] = WiFi.macAddress();
  doc["ip"] = WiFi.localIP().toString();
  char jsonChar[jsonsize];
  serializeJsonPretty(doc, jsonChar, jsonsize);
  server.send(200, "application/json", jsonChar);
}
void setwifi()
{
  server.send(200, "text/html", index_html);
}
void ssid()
{
  doc.clear();
  doc["ssid"] = WiFi.SSID();

  char jsonChar[jsonsize];
  serializeJsonPretty(doc, jsonChar, jsonsize);
  server.send(200, "applic ation/json", jsonChar);
}
void flip()
{
  uptime++;      //เวลา run
  otatime++;     //เพิ่มการ update
  checkintime++; //เพิ่มการนับ
  porttrick++;   //บอกว่า 1 วิละ
  readdhttime++; //บอกเวลา สำหรับอ่าน DHT
  restarttime++;
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
void setconfig()
{
  server.send(200, "text/html", setupconfig);
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
  EEPROM.put(ADDR + 100, configdata);
  EEPROM.commit();

  doc.clear();
  status();
  doc["message"] = "set value " + v + "TO " + value;
  char jsonChar[jsonsize];
  serializeJsonPretty(doc, jsonChar, jsonsize);
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Headers", "application/json");
  // 'Access-Control-Allow-Headers':'application/json'
  server.send(200, "application/json", jsonChar);
}
void reset()
{
  // StaticJsonDocument<1000> doc;
  // JsonObject portsobj = doc.createNestedObject("ports");
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

  char jsonChar[jsonsize];
  serializeJsonPretty(doc, jsonChar, jsonsize);
  server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  server.sendHeader("Access-Control-Allow-Headers", "application/json");
  // 'Access-Control-Allow-Headers':'application/json'
  server.send(200, "application/json", jsonChar);
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
  server.begin(); //เปิด TCP Server
  Serial.println("Server started");
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
    EEPROM.put(ADDR + 100, configdata);
    EEPROM.commit();
  }
  else
  {
    EEPROM.get(ADDR, wifidata);
    EEPROM.get(ADDR + 100, configdata);
  }
  setport();
  Serial.println();
  Serial.println("-----------------------------------------------");
  Serial.println(wifidata.ssid);
  Serial.println(wifidata.password);
  Serial.println("-----------------------------------------------");
  WiFiMulti.addAP(wifidata.ssid, wifidata.password);
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
  // WiFi.mode(WIFI_STA);

  // WiFiMulti.addAP("forpi3", "04qwerty");
  WiFiMulti.addAP("forpi", "04qwerty");
  WiFiMulti.addAP("forpi2", "04qwerty");
  WiFiMulti.addAP("forpi5", "04qwerty");
  WiFiMulti.addAP("forpi4", "04qwerty");
  WiFiMulti.addAP("Sirifarm", "0932154741");
  WiFiMulti.addAP("test", "12345678");
  // WiFiMulti.addAP("farm", "12345678");
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
  server.handleClient();
  // t.update();
  if (checkintime > 600 && counttime < 1)
  {
    checkintime = 0;
    checkin();
  }
  if (otatime > 60 && counttime < 1)
  {
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

  if (configdata.havedht && readdhttime > 120 && dhtbuffer.count < 1)
  {
    readdhttime = 0;
    message = "Read DHT";
    readDHT();
  }

  if (configdata.havetorestart & restarttime > 60)
  { //ใช้สำหรับ check ว่า ยังติดต่อ server ได้เปล่าถ้าได้ก็ผ่านไป
    printIPAddressOfHost("fw.pixka.me");
  }
}