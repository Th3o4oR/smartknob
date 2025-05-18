#pragma once

#include <variant>
#include <Arduino.h>
#include <assert.h>

#include "proto_gen/smartknob.pb.h"
#include "pages/page.h"
#include "input_type.h"

// Forward declarations
enum class PageType; // Need to match the definition in page.h

// Event definitions
struct PageChangeEvent {
    PageType new_page;
};
struct ConfigChangeEvent {
    PB_SmartKnobConfig config;
};
struct MotorCalibrationEvent {};

using PageEvent = std::variant<
    PageChangeEvent,
    ConfigChangeEvent,
    MotorCalibrationEvent
>;

template<typename EventT>
class EventBus {
public:
    EventBus() {
        queue_ = xQueueCreate(10, sizeof(EventT));
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
