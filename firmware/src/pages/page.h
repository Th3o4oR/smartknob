#pragma once

#include <Arduino.h>
#include <functional>
#include <variant>

#include "proto_gen/smartknob.pb.h"
#include "input_type.h"
#include "event_bus.h"

#include "logger.h"

enum class PageType {
    MAIN_MENU_PAGE = 0,
    LIGHTS_PAGE,
    SETTINGS_PAGE,
    MORE_PAGE,
    DEMO_PAGE,
    MEDIA_MENU_PAGE,
    VOLUME_PAGE,
};

namespace PageEvent {
    struct PageChange {
        PageType new_page;
    };
    struct ConfigChange {
        PB_SmartKnobConfig config;
    };
    struct MotorCalibration {};

    using Message = std::variant<
        PageChange,
        ConfigChange,
        MotorCalibration
    >;
    
    static_assert(std::is_pod_v<PageChange> == true);
    static_assert(std::is_pod_v<ConfigChange> == true);
    static_assert(std::is_pod_v<MotorCalibration> == true);
}

struct PageContext {
    EventSender<PageEvent::Message>& event_bus;
    Logger* logger;
};

class Page {
    public:
        Page(PageContext& context)
            : event_bus_(context.event_bus)
            , logger_(context.logger)
            {}
        virtual ~Page(){}

        virtual PB_SmartKnobConfig * getPageConfig() = 0;

        virtual void handleState(PB_SmartKnobState state) = 0;
        virtual void handleUserInput(input_t input, int input_data, PB_SmartKnobState state) = 0;

        void log(const std::string& msg) {
            logger_->log(msg);
        }
    
    protected:
        EventSender<PageEvent::Message>& event_bus_;

        void pageChange(PageType page) {
            event_bus_.publish(PageEvent::PageChange{page});
        }
        void configChange(PB_SmartKnobConfig& config) {
            event_bus_.publish(PageEvent::ConfigChange{config});
        }
        void motorCalibration() {
            event_bus_.publish(PageEvent::MotorCalibration{});
        }

        Logger *logger_;
};
