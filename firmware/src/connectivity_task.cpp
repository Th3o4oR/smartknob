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
            JsonDocument payload;
            DeserializationError deserialization_error = deserializeJson(payload, (char *)light_feed.lastread);
            if (deserialization_error) {
                LOG_ERROR("deserializeJson() returned %s", deserialization_error.c_str());
                return;
            }
            if (payload["brightness"].is<uint8_t>()) {
                uint8_t brightness = payload["brightness"]; // TODO: Replace with a command/message struct, like what is implemented for the motor_task
                for (auto listener : notification_listeners_) {
                    xQueueSend(listener, &brightness, portMAX_DELAY);
                    // xQueueOverwrite(listener, &brightness);
                }
            } else {
                LOG_ERROR("Brightness key not found in payload");
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
            // char log_msg[256];
            // snprintf(log_msg, sizeof(log_msg), "Published %s to MQTT", buffer);
            // log(log_msg);
            LOG_INFO("Published %s to MQTT", buffer);
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
            LOG_SUCCESS("Connected to the WiFi network. IP address: %s", WiFi.localIP().toString().c_str());
        }
        return true;
    }

    /*
    WL_NO_SHIELD
    WL_IDLE_STATUS
    WL_NO_SSID_AVAIL
    WL_SCAN_COMPLETED
    WL_CONNECTED
    WL_CONNECT_FAILED
    WL_CONNECTION_LOST
    WL_DISCONNECTED
    */
    // if (wifi_status != wifi_previous_status) {
    //     switch (wifi_status) {
    //         case WL_NO_SHIELD:
    //             LOG_WARN("No WiFi shield detected");
    //             break;
    //         case WL_IDLE_STATUS:
    //             LOG_INFO("WiFi idle status");
    //             break;
    //         case WL_NO_SSID_AVAIL:
    //             LOG_WARN("No SSID available");
    //             break;
    //         case WL_SCAN_COMPLETED:
    //             LOG_INFO("WiFi scan completed");
    //             break;
    //         case WL_CONNECTED:
    //             LOG_INFO("Connected to the WiFi network");
    //             break;
    //         case WL_CONNECT_FAILED:
    //             LOG_WARN("WiFi connection failed");
    //             break;
    //         case WL_CONNECTION_LOST:
    //             LOG_WARN("WiFi connection lost");
    //             break;
    //         case WL_DISCONNECTED:
    //             LOG_WARN("Disconnected from the WiFi network");
    //             break;
    //         default:
    //             LOG_ERROR("Unknown WiFi status");
    //             break;
    //     }
    // }

    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) { // Scan hasn't been triggered
        if (last_wifi_scan_ == 0 || millis() - last_wifi_scan_ > WIFI_SCAN_INTERVAL_MS) {
            LOG_INFO("Scanning for WiFi networks...");
            WiFi.scanNetworks(true); // First parameter specifies async scan
            // WiFi.scanNetworks(false); // Setting to false which introduces blocking
            last_wifi_scan_ = millis();
            return false;
        } else {
            return false;
        }
    } else if (n == WIFI_SCAN_RUNNING) {
        // log("WiFi scan still running");
        return false;
    }
    // if (n == 0) { // Should be handled by the target_network_found check
    //     log("No networks found");
    // }

    bool target_network_found = false;
    for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        LOG_INFO("%d: %s (%d) %s", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        if (WiFi.SSID(i) == WIFI_SSID) {
            target_network_found = true;
        }
    }
    WiFi.scanDelete();
    if (!target_network_found) {
        LOG_WARN("%s was not found in the scan. Another scan will be attempted in %d seconds.", WIFI_SSID, WIFI_SCAN_INTERVAL_MS / 1000);
        return false;
    }

    LOG_INFO("Attempting connection to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // While this is technically not blocking, some parts of the connection procedure will block. Consider disabling motor during startup, and moving to the second core?
    return false;
}

void ConnectivityTask::connectToMqttBroker() {
    int8_t ret;

    // Stop if already connected.
    if (mqtt.connected()) {
        return;
    }

    LOG_INFO("Connecting to MQTT... ");

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
    LOG_SUCCESS("MQTT Connected!");
}

void ConnectivityTask::addListener(QueueHandle_t queue) {
    notification_listeners_.push_back(queue);
}
