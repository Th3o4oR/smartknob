#include "connectivity_task.h"

WiFiClient client;

Adafruit_MQTT_Client  mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_PW);
Adafruit_MQTT_Publish feed_pub_discovery = Adafruit_MQTT_Publish(&mqtt, TOPIC_DISCOVERY);

Adafruit_MQTT_Publish feed_pub_lighting   = Adafruit_MQTT_Publish(&mqtt, TOPIC_PUBLISH_LIGHTING);
Adafruit_MQTT_Publish feed_pub_play_pause = Adafruit_MQTT_Publish(&mqtt, TOPIC_PUBLISH_PLAY_PAUSE);
Adafruit_MQTT_Publish feed_pub_skip       = Adafruit_MQTT_Publish(&mqtt, TOPIC_PUBLISH_SKIP);
Adafruit_MQTT_Publish feed_pub_volume     = Adafruit_MQTT_Publish(&mqtt, TOPIC_PUBLISH_VOLUME);

Adafruit_MQTT_Subscribe feed_sub_lighting = Adafruit_MQTT_Subscribe(&mqtt, TOPIC_SUBSCRIBE_BRIGHTNESS);

// Template overload to use std::variant with std::visit
// See https://stackoverflow.com/a/64018031
template<class... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overload(Ts...) -> overload<Ts...>;


void sendMQTTKnobDiscoveryMsg() {
    JsonDocument payload;
    char         buffer[512];

    payload["automation_type"] = "trigger";
    payload["type"]            = "action";
    payload["subtype"]         = "knob_input";
    payload["topic"]           = TOPIC_BASE;
    JsonObject device          = payload["device"].to<JsonObject>();
    device["name"]             = "SmartKnob";
    device["model"]            = "Model 1";
    device["sw_version"]       = "0.0.1";
    device["manufacturer"]     = SMARTKNOB_MANUFACTURER;
    JsonArray identifiers      = device["identifiers"].to<JsonArray>();
    identifiers.add(SMARTKNOB_ID);

    serializeJson(payload, buffer);
    feed_pub_discovery.publish(buffer);
}

void ConnectivityTask::receiveFromSubscriptions() {
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(0))) {
        if (subscription == &feed_sub_lighting) {
            JsonDocument         payload;
            DeserializationError deserialization_error = deserializeJson(payload, (char *)feed_sub_lighting.lastread);
            LOG_DEBUG("Received payload: %s", (char *)feed_sub_lighting.lastread);

            if (deserialization_error) {
                LOG_ERROR("deserializeJson() returned %s", deserialization_error.c_str());
                return;
            }

            bool state_present      = payload["state"].is<std::string>();
            bool brightness_present = payload["brightness"].is<uint8_t>();
            if (!state_present) {
                LOG_ERROR("State key not found in payload");
                return;
            }
            if (!brightness_present) {
                LOG_ERROR("Brightness key not found in payload");
                return;
            }
            
            BrightnessData brightness_payload = { .brightness = payload["brightness"] };
            if (payload["state"] == "OFF") { brightness_payload.brightness = 0; }
            dispatchToListeners(brightness_payload);
        }
    }
}

void ConnectivityTask::run() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    mqtt.subscribe(&feed_sub_lighting); // Subscriptions must be added before connecting to the broker

    while (1) {
        // When either WiFi or MQTT are not connected and the queue is full, any task attempting
        // to send messages will be blocked and won't run until space is available.
        // In order to avoid this, we need to clear the queue before attempting to connect to WiFi or MQTT.
        // The last message will be kept in the queue.
        // while (uxQueueMessagesWaiting(transmit_queue_) > 1) {
        //     MQTTPayload message;
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

        MQTTPayload mqtt_payload;
        if (xQueueReceive(transmit_queue_, &mqtt_payload, 0) == pdTRUE) {
            JsonDocument json_payload;
            char         buffer[256];
            
            Adafruit_MQTT_Publish *feed_pub = nullptr;
            auto visitor = overload {
                [&](const BrightnessData& p) {
                    json_payload["brightness"] = p.brightness;
                    feed_pub = &feed_pub_lighting;
                },
                [&](const PlayPauseData& p) {
                    json_payload["paused"] = p.paused;
                    feed_pub = &feed_pub_play_pause;
                },
                [&](const SkipData& p) {
                    json_payload["forward"] = p.forward;
                    feed_pub = &feed_pub_skip;
                },
            };
            std::visit(visitor, mqtt_payload);
            if (feed_pub == nullptr) {
                LOG_ERROR("No MQTT feed specified for publishing");
                continue;
            }
            
            serializeJson(json_payload, buffer);
            feed_pub->publish(buffer);
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
void ConnectivityTask::sendMqttMessage(MQTTPayload message) {
    if (!mqtt.connected()) {
        LOG_ERROR("MQTT not connected. Message will not be sent.");
        return;
    }
    xQueueSend(transmit_queue_, &message, portMAX_DELAY);
}

bool ConnectivityTask::initWiFi() {
    static wl_status_t wifi_previous_status;
    static wl_status_t wifi_status;
    wifi_previous_status = wifi_status;
    wifi_status          = WiFi.status();
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
        sendMQTTKnobDiscoveryMsg();
        return true;
    }

    return false;
}

/**
 * @brief Register a listener for a specific type of data.
 * 
 * @param subscription The type of data to listen for.
 * @param queue The queue handle to register as a listener.
 */
void ConnectivityTask::registerListener(MQTTSubscriptionType subscription, QueueHandle_t queue) {
    listeners_[subscription].push_back(queue);
}

/**
 * @brief Dispatch data to all listeners of the specified type.
 * 
 * @param data The data to dispatch.
 */
void ConnectivityTask::dispatchToListeners(const MQTTPayload& data) {
    // Match the type of data to the subscription type
    MQTTSubscriptionType subscription;
    auto visitor = overload {
        [&](const BrightnessData&) { subscription = MQTTSubscriptionType::LIGHTING; },
        [&](const PlayPauseData&) { subscription = MQTTSubscriptionType::PLAY_PAUSE; },
        [&](const SkipData&) { subscription = MQTTSubscriptionType::SKIP; },
        [&](auto&) {
            LOG_ERROR("Unknown data type");
            return;
        }
    };
    std::visit(visitor, data);
    
    auto it = listeners_.find(subscription);
    if (it != listeners_.end()) {
        for (auto& queue : it->second) {
            xQueueSend(queue, &data, portMAX_DELAY);
        }
    }
}
