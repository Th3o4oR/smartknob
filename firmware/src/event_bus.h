#pragma once

#include <Arduino.h>
#include <variant>
#include <assert.h>

#include "proto_gen/smartknob.pb.h"
#include "input_type.h"

template <typename EventT>
class EventBusCore {
  public:
    EventBusCore(size_t queue_size = 5) {
        queue_ = xQueueCreate(queue_size, sizeof(EventT));
        assert(queue_ != NULL);
    }

    ~EventBusCore() {
        vQueueDelete(queue_);
    }

    QueueHandle_t queue() const { return queue_; }

  private:
    QueueHandle_t queue_;
};

template <typename EventT>
class EventSender {
  public:
    explicit EventSender(QueueHandle_t queue)
        : queue_(queue) {}

    void publish(const EventT &e) const {
        BaseType_t result = xQueueSend(queue_, &e, 0);
        if (result != pdPASS) {
            // Handle error
        }
    }

  private:
    QueueHandle_t queue_;
};

template <typename EventT>
class EventReceiver {
  public:
    explicit EventReceiver(QueueHandle_t queue)
        : queue_(queue) {}

    bool receive(EventT &e, TickType_t timeout = 0) const {
        return xQueueReceive(queue_, &e, timeout) == pdPASS;
    }

  private:
    QueueHandle_t queue_;
};
