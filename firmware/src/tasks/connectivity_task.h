#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

#include <vector>
#include <map>
#include <variant>
#include <unordered_map>
#include <typeindex>

#include "task.h"
#include "secrets.h" // SMARTKNOB_ID

#define TOPIC_DISCOVERY        "homeassistant/device_automation/knob_" SMARTKNOB_ID "/action_knob/config"
#define TOPIC_BASE             "knob/" SMARTKNOB_ID
#define TOPIC_PUBLISH_PREFIX   TOPIC_BASE "/action"
#define TOPIC_SUBSCRIBE_PREFIX TOPIC_BASE "/set"

#define TOPIC_PUBLISH_LIGHTING   TOPIC_PUBLISH_PREFIX "/lighting"
#define TOPIC_PUBLISH_PLAY_PAUSE TOPIC_PUBLISH_PREFIX "/play_pause"
#define TOPIC_PUBLISH_SKIP       TOPIC_PUBLISH_PREFIX "/skip"
#define TOPIC_PUBLISH_VOLUME     TOPIC_PUBLISH_PREFIX "/volume"

#define TOPIC_SUBSCRIBE_BRIGHTNESS "zigbee2mqtt/Bed facing"

struct ColorData { uint8_t r, g, b; };
struct BrightnessData { int brightness; };
struct PlayPauseData { bool paused; };
struct SkipData { bool forward; };

enum class MQTTSubscriptionType {
    LIGHTING,
    PLAY_PAUSE,
    SKIP,
    VOLUME,
};

using MQTTPayload = std::variant<
    // ColorData,
    BrightnessData,
    PlayPauseData,
    SkipData
>;

static constexpr uint32_t WIFI_SCAN_INTERVAL_MS       = 60 * 1000;
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS     = 5 * 1000;
static constexpr uint32_t MQTT_CONNECTION_INTERVAL_MS = 5 * 1000;
static constexpr uint32_t TRANSMISSION_QUEUE_SIZE     = 8;

class ConnectivityTask : public Task<ConnectivityTask> {
    friend class Task<ConnectivityTask>; // Allow base Task to invoke protected run()

  public:
    ConnectivityTask(const uint8_t task_core, const uint32_t stack_depth)
        : Task("Connectivity", stack_depth, 1, task_core)
        , transmit_queue_(xQueueCreate(TRANSMISSION_QUEUE_SIZE, sizeof(MQTTPayload)))
    {
        assert(transmit_queue_ != NULL);
    }
    ~ConnectivityTask() {
        vQueueDelete(transmit_queue_);
    }

    void sendMqttMessage(MQTTPayload message);
    void receiveFromSubscriptions();

    void registerListener(MQTTSubscriptionType subscription, QueueHandle_t queue);

  protected:
    void run();

  private:
    uint32_t last_wifi_scan_               = 0;
    uint32_t last_mqtt_connection_attempt_ = 0;

    std::unordered_map<MQTTSubscriptionType, std::vector<QueueHandle_t>> listeners_;
    
    QueueHandle_t transmit_queue_;

    void dispatchToListeners(const MQTTPayload& data);
    
    bool initWiFi();
    bool connectToMqttBroker();
};
