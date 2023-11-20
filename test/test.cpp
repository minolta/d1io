#include <Arduino.h>
#include <unity.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <ESP8266httpUpdate.h>
#define UNITY_INCLUDE_DOUBLE
#define UNITY_DOUBLE_PRECISION 1e-12
// #include <LITTLEFS.h>
#include <ArduinoJson.h>
#include "Configfile.h"
#include <ESP8266WiFi.h>
const char *ssid = "ESP8266-Access-Point";
const char *password = "123456789";
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
const char getpassword_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Config file wifi</h2>
  <form action="/setpassword" method="POST" enctype="application/x-www-form-urlencoded">
  <input type="text" name="ssid" >
  <input type="password" name="password">
   <input type="submit" value="Send">
</form>
</body>
</html>)rawliteral";
String processor(const String &var)
{
  //   //Serial.println(var);
  //   if(var == "TEMPERATURE"){
  //     return String(t);
  //   }
  //   else if(var == "HUMIDITY"){
  //     return String(h);
  //   }
  return String();
}
void Testwrfile(void)
{
  Configfile cc("/testrw.cfg");
  cc.openFile();

  cc.addConfig("doublevalue", "2.2");
  cc.addConfig("newconfig", "for test only");
  double p = cc.getDobuleConfig("doublevalue", 2.2);
  String s = cc.getConfig("newconfig");
  // TEST_ASSERT_EQUAL_DOUBLE(2.2, p);
  Serial.printf("\n\nTEST ******************** %.2f ######## %s\n", p, s.c_str());
}
void connect()
{
  WiFi.mode(WIFI_STA);

  WiFi.begin("forpi", "04qwerty");
  Serial.print("connect.");
  int ft = 0;
  // display.clear();
  while (WiFi.status() != WL_CONNECTED) // รอการเชื่อมต่อ
  {
    delay(250);

    ft++;
    if (ft > 300)
    {
      Serial.println("Connect main wifi timeout");
      // apmode = 1;
      break;
    }
    Serial.print(".");
  }
}
void testApMode()
{
  Configfile cfg("/testwifi.cfg");
  cfg.openFile();
  AsyncWebServer server(80);

  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("           AP IP address: ");
  Serial.println(IP);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", getpassword_html, processor); });

  server.on("/setpassword", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              String password = request->arg("password");
              String ssid = request->arg("ssid");

              request->send(200, "text/html", "Ok SSID " + ssid + " Password " + password);
              ESP.restart(); });
  server.begin();
  delay(60000 * 5);
}
void testConnecttomaster()
{
  connect();
  WiFiClient client;
  HTTPClient http;

  // HTTPClient http; // Declare object of class HTTPClient
  // String h = cfg.getConfig("checkinurl", "http://192.168.88.225:888/hello");
  // int httpCode = http.GET(h);        // Send the request
  // String payload = http.getString(); // Get the response payload
  Serial.print(" Http Code:");
  http.begin(client, "http://192.168.88.225:888/hello/192.168.0.1/10000/test");
  int httpResponseCode = http.GET();
  Serial.println(httpResponseCode);
  Serial.println(http.getString());
}
void updatefw()
{
  connect();
  WiFiClient client;
  String updateurl = "http://192.168.88.225:2000/rest/fw/update/d1io/";
  String u  = updateurl + "0";
// String u = "http://192.168.88.225:2000/rest/fw/update/d1io/0";

  Serial.println("CALL " + u);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, u);
  // t_httpUpdate_return ret = ESPhttpUpdate.update(client,updateurl,"0");
    // t_httpUpdate_return ret = ESPhttpUpdate.update(client, "127.0.0.1", 2000, "/rest/fw/update/d1io/0", "0");
  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    // Serial.println("[update] Update failed.");
     Serial.printf("[update] Update failed (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("[update] Update no Update.");
    break;
  case HTTP_UPDATE_OK:
    Serial.println("[update] Update ok."); // may not called we reboot the ESP
    break;
  }
}
void setup()
{

  Serial.begin(9600);
  // NOTE!!! Wait for >2 secs
  // if board doesn't support software reset via Serial.DTR/RTS
  // delay(2000);

  UNITY_BEGIN();
  // RUN_TEST(Testwrfile);
  // RUN_TEST(testApMode);
  // RUN_TEST(testConnecttomaster);
  // RUN_TEST(updatefw);
  // RUN_TEST(testLoad);
  // RUN_TEST(getDefaultAlreadyhave);
  // RUN_TEST(testNull);
  // RUN_TEST(testGetIntconfig);
  // RUN_TEST(testGetIntconfigbydefaultint);
  // RUN_TEST(testGetDoubleconfigbydefaultdouble);
  // RUN_TEST(testDoublenull);
  // RUN_TEST(addInt);
  // RUN_TEST(addDouble);
  // RUN_TEST(TestloadConfig);
  // RUN_TEST(getDefault);
  // RUN_TEST(open);
  // RUN_TEST(Testloadall);
  // RUN_TEST(getFilename);
  // RUN_TEST(readValue);
  // RUN_TEST(makedoc);
  // RUN_TEST(readConfig);
  // RUN_TEST(TestAddConfig);
  // RUN_TEST(TestGetvalue);
  // RUN_TEST(havefile);
  // TEST_ASSERT_EQUAL(true, LITTLEFS.begin(true));
  UNITY_END();
}

void loop()
{
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);
  delay(500);
}