#include "config_desc.h"
#include <string.h>

struct ConfigDescItem
{
    const char *key;
    const char *desc;
};

static const ConfigDescItem CONFIG_DESC[] = {
    {"ssid", "WiFi network name to connect to"},
    {"password", "WiFi password"},
    {"wifitimeout", "Seconds before WiFi reconnect (default 60)"},
    {"maxconnecttimeout", "Max WiFi connect attempts (default 60)"},
    {"havetorestart", "1 = restart if WiFi fails repeatedly"},
    {"apmoderun", "AP mode run time minutes"},
    {"logslots", "In-memory log ring size 4-48 (default 16)"},
    {"checkinurl", "Server URL for check-in POST"},
    {"checkintime", "Check-in interval seconds (default 600)"},
    {"checkconnectiontime", "Connection health check interval (default 600)"},
    {"otaurl", "Firmware OTA update URL base"},
    {"otahost", "OTA host name"},
    {"otatime", "OTA check interval seconds (default 600)"},
    {"updatetimestampurl", "URL to sync device timestamp"},
    {"timezone", "Timezone offset seconds (default 25000)"},
    {"D5mode", "D5 pin mode: 0=INPUT 1=OUTPUT"},
    {"D5initvalue", "D5 initial output: 0=LOW 1=HIGH"},
    {"D6mode", "D6 pin mode: 0=INPUT 1=OUTPUT"},
    {"D6initvalue", "D6 initial output: 0=LOW 1=HIGH"},
    {"D7mode", "D7 pin mode: 0=INPUT 1=OUTPUT"},
    {"D7initvalue", "D7 initial output: 0=LOW 1=HIGH"},
    {"D8mode", "D8 pin mode: 0=INPUT 1=OUTPUT"},
    {"D8initvalue", "D8 initial output: 0=LOW 1=HIGH"},
    {"havedht", "1 = enable DHT temp/humidity sensor"},
    {"havesht", "1 = enable SHT temp/humidity sensor"},
    {"haveds", "1 = enable DS18B20 temperature sensor"},
    {"havea0", "1 = enable analog A0"},
    {"havesoisensor", "1 = enable soil moisture on A0"},
    {"va0", "A0 voltage offset"},
    {"sensorvalue", "Sensor scale factor"},
    {"senservalue", "Sensor scale factor (legacy key name)"},
    {"readdhttime", "DHT read interval seconds"},
    {"ntpupdatetime", "NTP sync interval seconds"},
    {"restarttime", "Auto-restart interval seconds"},
    {"readnextsoi", "Soil read interval milliseconds"},
    {"airvalue", "A0 raw value in dry air (/findair)"},
    {"wetvalue", "A0 raw value in water (/findwet)"},
};

static const char DESC_DEFAULT[] = "Custom config key";

const char *configDescFor(const char *key)
{
    if (key == nullptr)
        return DESC_DEFAULT;
    const size_t n = sizeof(CONFIG_DESC) / sizeof(CONFIG_DESC[0]);
    for (size_t i = 0; i < n; i++)
    {
        if (strcmp(CONFIG_DESC[i].key, key) == 0)
            return CONFIG_DESC[i].desc;
    }
    return DESC_DEFAULT;
}

String configRowHtml(const String &k, const String &v)
{
    return String("<tr><td>") + k + "</td><td class=\"desc\">" + configDescFor(k.c_str()) +
           "</td><td><label class=\"val\" id=" + k + "value>" + v +
           "</label></td><td><input id=" + k + " type=\"text\" value=\"" + v +
           "\"></td><td><button class=\"btn btn-set\" type=\"button\" onClick=\"setvalue(this,'" + k +
           "','" + v + "')\">Save</button></td><td><button class=\"btn btn-rm\" type=\"button\" onClick=\"remove('" +
           k + "')\">Del</button></td></tr>";
}
