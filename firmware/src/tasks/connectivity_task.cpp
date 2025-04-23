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
    transmit_queue_ = xQueueCreate(1, sizeof(Message));
    assert(transmit_queue_ != NULL);
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
    device["manufacturer"]     = SMARTKNOB_MANUFACTURER;
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
            LOG_DEBUG("Received payload: %s", (char *)light_feed.lastread);
            
            if (deserialization_error) {
                LOG_ERROR("deserializeJson() returned %s", deserialization_error.c_str());
                return;
            }

            bool state_present = payload["state"].is<std::string>();
            bool brightness_present = payload["brightness"].is<uint8_t>();
            if (!state_present) {
                LOG_ERROR("State key not found in payload");
            }
            if (!brightness_present) {
                LOG_ERROR("Brightness key not found in payload");
            }

            bool lights_turned_off = (state_present && payload["state"] == "OFF");
            if (lights_turned_off) {
                bool state = false;
                for (auto listener : state_listeners_) {
                    xQueueSend(listener, &state, portMAX_DELAY);
                }
            } else if (brightness_present) {
                uint8_t brightness = payload["brightness"]; // TODO: Replace with a command/message struct, like what is implemented for the motor_task
                for (auto listener : brightness_listeners_) {
                    xQueueSend(listener, &brightness, portMAX_DELAY);
                }
            }
        }
    }
}

void ConnectivityTask::run() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    mqtt.subscribe(&light_feed); // Subscriptions must be added before connecting to the broker

    while (1) {
        // When either WiFi or MQTT are not connected and the queue is full, any task attempting
        // to send messages will be blocked and won't run until space is available.
        // In order to avoid this, we need to clear the queue before attempting to connect to WiFi or MQTT.
        // The last message will be kept in the queue.
        // while (uxQueueMessagesWaiting(transmit_queue_) > 1) {
        //     Message message;
        //     xQueueReceive(transmit_queue_, &message, 0);
        // }
        
        if (!initWiFi()) {
            delay(10);
            continue;
        }

        if (!connectToMqttBroker()) {
            delay(10);
            continue;
        }

        Message message;
        if (xQueueReceive(transmit_queue_, &message, 0) == pdTRUE) {
            JsonDocument payload;
            char         buffer[256];
            payload["trigger_name"]  = message.trigger_name;
            payload["trigger_value"] = message.trigger_value;
            serializeJson(payload, buffer);

            action_feed.publish(buffer);
            LOG_INFO("Published %s to MQTT", buffer);
        }

        receiveFromSubscriptions();

        delay(100);
    }
}

/**
 * @brief Send a message to the MQTT broker.
 * 
 * This function overwrites the queue with the new message.
 * 
 * @param message The message to send.
 */
void ConnectivityTask::sendMqttMessage(Message message) {
    xQueueOverwrite(transmit_queue_, &message);
}

bool ConnectivityTask::initWiFi() {
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

bool ConnectivityTask::connectToMqttBroker() {
    // Stop if already connected.
    if (mqtt.connected()) {
        return true;
    }

    if (last_mqtt_connection_attempt_ == 0 || millis() - last_mqtt_connection_attempt_ > MQTT_CONNECTION_INTERVAL_MS) {
        last_mqtt_connection_attempt_ = millis();
        LOG_INFO("Connecting to MQTT... ");
        uint8_t ret = mqtt.connect();
        if (ret != 0) {
            LOG_ERROR("MQTT connection failed: %s", mqtt.connectErrorString(ret));
            return false;
        }

        LOG_SUCCESS("MQTT Connected!");
        sendMqttKnobStateDiscoveryMsg();
        return true;
    }

    return false;
}

void ConnectivityTask::addBrightnessListener(QueueHandle_t queue) {
    brightness_listeners_.push_back(queue);
}
void ConnectivityTask::addStateListener(QueueHandle_t queue) {
    state_listeners_.push_back(queue);
}
