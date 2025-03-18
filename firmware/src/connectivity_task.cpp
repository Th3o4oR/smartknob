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
String                light_topic     = "zigbee2mqtt/Bed facing";
Adafruit_MQTT_Publish discovery_feed  = Adafruit_MQTT_Publish(&mqtt, discovery_topic.c_str());
Adafruit_MQTT_Publish action_feed     = Adafruit_MQTT_Publish(&mqtt, action_topic.c_str());
Adafruit_MQTT_Subscribe light_feed    = Adafruit_MQTT_Subscribe(&mqtt, light_topic.c_str());

ConnectivityTask::ConnectivityTask(const uint8_t task_core, const uint32_t stack_depth)
    : Task("Connectivity", stack_depth, 1, task_core) {
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

void ConnectivityTask::receiveFromSubscriptions() {
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(0))) {
        if (subscription == &light_feed) {
            // char buf[512];
            // snprintf(buf, sizeof(buf), "Got: %s", (char *)light_feed.lastread);
            // log(buf);

            JsonDocument payload;
            DeserializationError deserialization_error = deserializeJson(payload, (char *)light_feed.lastread);
            if (deserialization_error) {
                String deserialization_error_str = "deserializeJson() returned " + String(deserialization_error.c_str());
                log(deserialization_error_str.c_str());
                return;
            }
            if (payload["brightness"].is<uint8_t>()) {
                uint8_t brightness = payload["brightness"]; // TODO: Replace with a command/message struct, like what is implemented for the motor_task
                for (auto listener : notification_listeners_) {
                    xQueueSend(listener, &brightness, portMAX_DELAY);
                    // xQueueOverwrite(listener, &brightness);
                }
            } else {
                log("Brightness key not found in payload");
            }
        }
    }
}

void ConnectivityTask::run() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(2000); // Initial delay 

    mqtt.subscribe(&light_feed);

    while (1) {
        if (!initWiFi()) {
            delay(10);
            continue;
        }
        if (!mqtt.connected()) {
            connectToMqttBroker();
            sendMqttKnobStateDiscoveryMsg();
        }

        Message message;
        if (xQueueReceive(queue_, &message, 0) == pdTRUE) {
            JsonDocument payload;
            char         buffer[256];
            payload["trigger_name"]  = message.trigger_name;
            payload["trigger_value"] = message.trigger_value;
            serializeJson(payload, buffer);

            action_feed.publish(buffer);

            // Log value published
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Published %s to MQTT", buffer);
            log(log_msg);
        }

        receiveFromSubscriptions();

        delay(100);
    }
}

void ConnectivityTask::sendMqttMessage(Message message) {
    xQueueSend(queue_, &message, portMAX_DELAY);
    // xQueueOverwrite(queue_, &message);
}

bool ConnectivityTask::initWiFi() {
    char buf[200];
    static wl_status_t wifi_previous_status;
    static wl_status_t wifi_status;
    wifi_previous_status = wifi_status;
    wifi_status = WiFi.status();
    if (wifi_status == WL_CONNECTED) {
        if (wifi_previous_status != WL_CONNECTED) {
            log(buf);
        }
        return true;
    }

    if (wifi_connecting_) {
        if (millis() - wifi_connecting_start_ > WIFI_CONNECT_TIMEOUT_MS) {
            log("WiFi connection timed out");
            wifi_connecting_ = false;
        } else {
            log("WiFi connecting...");
            return false;
        }
    }

    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) { // Scan hasn't been triggered
        // log("WiFi scan not triggered");
        if (last_wifi_scan_ == 0 || millis() - last_wifi_scan_ > WIFI_SCAN_INTERVAL_MS) {
            WiFi.scanNetworks(true); // First parameter specifies async scan
            // WiFi.scanNetworks(false); // Setting to false which introduces blocking
            last_wifi_scan_ = millis();
            return false;
        } else {
            // char buf[200];
            // snprintf(buf, sizeof(buf), "Next WiFi scan in %d seconds", (WIFI_SCAN_INTERVAL_MS - (millis() - last_wifi_scan_)) / 1000);
            // log(buf);
            return false;
        }
    } else if (n == WIFI_SCAN_RUNNING) {
        // log("WiFi scan still running");
        return false;
    }
    // if (n == 0) { // Should be handled by the target_network_found check
    //     log("No networks found");
    // }

    char buf[200];
    bool target_network_found = false;
    for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        snprintf(buf, sizeof(buf), "%d: %s (%d) %s", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        log(buf);
        if (WiFi.SSID(i) == WIFI_SSID) {
            target_network_found = true;
        }
    }
    WiFi.scanDelete();
    if (!target_network_found) {
        snprintf(buf, sizeof(buf), "%s was not found in the scan. Another scan will be attempted in %d seconds.", WIFI_SSID, WIFI_SCAN_INTERVAL_MS / 1000);
        log(buf);
        return false;
    }

    snprintf(buf, sizeof(buf), "Attempting connection to %s", WIFI_SSID);
    log(buf);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    return false;
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

void ConnectivityTask::addListener(QueueHandle_t queue) {
    notification_listeners_.push_back(queue);
}

void ConnectivityTask::setLogger(Logger *logger) {
    logger_ = logger;
}

void ConnectivityTask::log(const char *msg) {
    if (logger_ != nullptr) {
        logger_->log(msg);
    }
}
