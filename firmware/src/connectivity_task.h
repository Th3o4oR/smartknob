#pragma once

#include <Arduino.h>
#include "task.h"
// #include "logger.h"
#include <vector>

static constexpr uint32_t WIFI_SCAN_INTERVAL_MS = 60 * 1000;
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 5 * 1000;

struct Message {
    String trigger_name;
    int trigger_value;
};

class ConnectivityTask : public Task<ConnectivityTask> {
    friend class Task<ConnectivityTask>; // Allow base Task to invoke protected run()

    public:
        ConnectivityTask(const uint8_t task_core, const uint32_t stack_depth);
        ~ConnectivityTask();

        void addListener(QueueHandle_t queue);
        void sendMqttMessage(Message message);
        void receiveFromSubscriptions();

    protected:
        void run();

    private:
        uint32_t last_wifi_scan_ = 0;
        
        std::vector<QueueHandle_t> notification_listeners_;

        QueueHandle_t queue_;

        bool initWiFi();
        void connectToMqttBroker();
};
