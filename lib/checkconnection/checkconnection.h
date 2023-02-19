#include <Arduino.h>
#include "Configfile.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#ifndef CHECKCONNECTION_H
#define CHECKCONNECTION_H
int talktoServer(String ip, String name, long uptime,Configfile *p);
#endif