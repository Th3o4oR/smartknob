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

struct PageContext {
    EventBus<PageEvent>& event_bus;
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
        EventBus<PageEvent>& event_bus_;

        void pageChange(PageType page) {
            event_bus_.publish(PageChangeEvent{page});
        }
        void configChange(PB_SmartKnobConfig& config) {
            event_bus_.publish(ConfigChangeEvent{config});
        }
        void motorCalibration() {
            event_bus_.publish(MotorCalibrationEvent{});
        }
        // void pageChange(PageType page) {
        //     event_bus_.publish(PageEvent {
        //         .type = PageEventType::PAGE_CHANGE,
        //         .data = {
        //             .page_change = {page}
        //         },
        //     });
        // }
        // void configChange(PB_SmartKnobConfig config) {
        //     PB_SmartKnobConfig *config_ptr = new PB_SmartKnobConfig(config);
        //     event_bus_.publish(PageEvent {
        //         .type = PageEventType::CONFIG_CHANGE,
        //         .data = {
        //             .config_change = {config_ptr}
        //         },
        //     });
        // }
        // void motorCalibration() {
        //     event_bus_.publish(PageEvent {
        //         .type = PageEventType::MOTOR_CALIBRATION,
        //         .data = {
        //             .motor_calibration = {}
        //         },
        //     });
        // }

        Logger *logger_;
};
