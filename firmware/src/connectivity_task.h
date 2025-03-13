#pragma once

#include <Arduino.h>
#include "task.h"
#include "logger.h"

struct Message {
    String trigger_name;
    int trigger_value;
};

class ConnectivityTask : public Task<ConnectivityTask> {
    friend class Task<ConnectivityTask>; // Allow base Task to invoke protected run()

    public:
        ConnectivityTask(const uint8_t task_core, const uint32_t stack_depth);
        ~ConnectivityTask();

        void setLogger(Logger *logger);
        void sendMqttMessage(Message message);

    protected:
        void run();

    private:
        QueueHandle_t queue_;
        Logger       *logger_;

        void initWiFi();
        void connectToMqttBroker();
        void log(const char *msg);
};
