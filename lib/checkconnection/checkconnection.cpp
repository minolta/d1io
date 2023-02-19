#include "checkconnection.h"

int talktoServer(String ip, String name, long uptime, Configfile *p)
{
  String pt = p->getConfig("talkurl", "http://192.168.88.21:3334/hello");

  String talkurl = pt + "/" + WiFi.localIP().toString() + "/" + uptime + "/" + name;
  WiFiClient client;
  HTTPClient http;

  // HTTPClient http; // Declare object of class HTTPClient
  // String h = cfg.getConfig("checkinurl", "http://192.168.88.225:888/hello");
  // int httpCode = http.GET(h);        // Send the request
  // String payload = http.getString(); // Get the response payload
  Serial.print(" Http Code:");
  http.begin(client, talkurl);
  int httpResponseCode = http.GET();
  Serial.println(httpResponseCode);
  Serial.println(http.getString());
  return httpResponseCode;
}