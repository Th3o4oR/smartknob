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

static_assert(std::is_pod_v<PageChangeEvent> == true);
static_assert(std::is_pod_v<ConfigChangeEvent> == true);
static_assert(std::is_pod_v<MotorCalibrationEvent> == true);

// This definition is commented out because we are using std::variant instead
// Uncomment if you want to use a union instead of std::variant, which is POD
// enum class PageEventType {
//     PAGE_CHANGE,
//     CONFIG_CHANGE,
//     MOTOR_CALIBRATION,
// };
// struct PageEvent {
//     PageEventType type;
//     union {
//         PageChangeEvent page_change;
//         ConfigChangeEvent config_change;
//         MotorCalibrationEvent motor_calibration;
//     } data;
// };
// static_assert(std::is_pod_v<PageEvent> == true);

using PageEvent = std::variant<
    PageChangeEvent,
    ConfigChangeEvent,
    MotorCalibrationEvent
>;
// static_assert(std::is_pod_v<PageEvent>); // Variant is not POD, but we can use it as a union of POD types

template<typename EventT>
class EventBus {
public:
    // Event bus used to send and receive page-related events between components.
    //
    // ⚠️ Important:
    // This must be initialized explicitly in the constructor's member initializer list.
    // On ESP32 (with FreeRTOS), creating queues too early—such as during automatic
    // default construction of class members—can cause spinlock assertions if the scheduler
    // or core subsystems are not fully ready. Initializing it in the initializer list ensures
    // it is constructed after the RTOS environment is properly set up.
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
