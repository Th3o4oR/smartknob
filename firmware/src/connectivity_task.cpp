#include "connectivity_task.h"
#include "WiFi.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT_Client.h"
#include "ArduinoJson.h"
#include "secrets.h"

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_PW);

String smartknob_id = String(SMARTKNOB_ID);

String                discovery_topic = "homeassistant/device_automation/knob_" + smartknob_id + "/action_knob/config";
String                action_topic    = "knob/" + smartknob_id + "/action";
Adafruit_MQTT_Publish discovery_feed  = Adafruit_MQTT_Publish(&mqtt, discovery_topic.c_str());
Adafruit_MQTT_Publish action_feed     = Adafruit_MQTT_Publish(&mqtt, action_topic.c_str());

ConnectivityTask::ConnectivityTask(const uint8_t task_core)
    : Task("Connectivity", 4096, 1, task_core) {
  queue_ = xQueueCreate(5, sizeof(Message));
  assert(queue_ != NULL);
}

ConnectivityTask::~ConnectivityTask() {}

void sendMqttKnobStateDiscoveryMsg() {
  JsonDocument payload;
  char         buffer[512];

  payload["automation_type"] = "trigger";
  payload["type"]            = "action";
  payload["subtype"]         = "knob_input";
  payload["topic"]           = action_topic;
  JsonObject device          = payload["device"].to<JsonObject>();
  device["name"]             = "SmartKnob";
  device["model"]            = "Model 1";
  device["sw_version"]       = "0.0.1";
  device["manufacturer"]     = "lgc00";
  JsonArray identifiers      = device["identifiers"].to<JsonArray>();
  identifiers.add(smartknob_id);

  // size_t n = serializeJson(payload, buffer);
  serializeJson(payload, buffer);

  discovery_feed.publish(buffer);
}

void ConnectivityTask::run() {
  initWiFi();

  connectToMqttBroker();
  sendMqttKnobStateDiscoveryMsg();

  while (1) {
    // Serial.print("Checking for messages...");
    connectToMqttBroker();

    Message message;
    if (xQueueReceive(queue_, &message, 0) == pdTRUE) {
      JsonDocument payload;
      char         buffer[256];
      payload["trigger_name"]  = message.trigger_name;
      payload["trigger_value"] = message.trigger_value;
      serializeJson(payload, buffer);

      action_feed.publish(buffer);

      log("Published message to MQTT");
    }

    delay(1);
  }
}

void ConnectivityTask::sendMqttMessage(Message message) {
  xQueueSend(queue_, &message, portMAX_DELAY);
}

void ConnectivityTask::initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  // delay(100);

  log("Starting scan");
  int n = WiFi.scanNetworks();

  char buf[200];

  if (n == 0) {
    log("No networks found");
  } else {
    snprintf(buf, sizeof(buf), "%d networks found", n);
    log(buf);
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      snprintf(buf, sizeof(buf), "%d: %s (%d) %s", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      log(buf);
    }
  }

  // TODO: Only connect to WiFi if it appears in the scan
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    log("Connecting to WiFi...");
    delay(1000);
  }
  // Log Local IP
  log("Connected to the WiFi network");
  snprintf(buf, sizeof(buf), "IP address: %s", WiFi.localIP().toString().c_str());
  log(buf);
}

void ConnectivityTask::connectToMqttBroker() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  log("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    log(String(mqtt.connectErrorString(ret)).c_str());
    log("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1)
        ;
    }
  }
  log("MQTT Connected!");
}

void ConnectivityTask::setLogger(Logger *logger) {
  logger_ = logger;
}

void ConnectivityTask::log(const char *msg) {
  if (logger_ != nullptr) {
    logger_->log(msg);
  }
}
