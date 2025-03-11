/*
  Clone this file and rename it to secrets.h
  Fill in the values below with your own secrets
  secrets.h has been added to .gitignore so it won't be commited to git
*/
#define WIFI_SSID "some_ssid"
#define WIFI_PASSWORD "some_password"

#define MQTT_SERVER        "192.168.1.1"
#define MQTT_SERVERPORT    1883
#define MQTT_USERNAME      "some_user"
#define MQTT_PW            "some_password"
#define MQTT_COMMAND_TOPIC "smartknob"

#define SMARTKNOB_ID "sk1" // id used for MQTT autodiscovery - needs to be unique for each device
