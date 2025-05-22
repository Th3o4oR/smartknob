#pragma once

#include <variant>
#include <Arduino.h>
#include <assert.h>

#include "proto_gen/smartknob.pb.h"
#include "input_type.h"

template<typename EventT>
class EventBus {
public:
    // Important:
    // This must be initialized explicitly in the constructor's member initializer list.
    // Initialization too early may cause the queue to be created before the FreeRTOS scheduler is started.
    EventBus() {
        queue_ = xQueueCreate(5, sizeof(EventT));
        assert(queue_ != NULL);
    }
    ~EventBus() {
        vQueueDelete(queue_);
    }

    void publish(const EventT& e) {
        BaseType_t result = xQueueSend(queue_, &e, 0);
        if (result != pdPASS) {
            // Handle error: queue full or send failed
        }
    }
    bool poll(EventT& e, TickType_t timeout = 0) {
        return xQueueReceive(queue_, &e, timeout) == pdPASS;
    }

private:
    QueueHandle_t queue_;
};
