#pragma once

#include <functional>

#include "../proto_gen/smartknob.pb.h"
#include "../input_type.h"

#include "interface_callbacks.h"
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

class Page {
    public:
        Page(PageChangeCallback page_change_callback,
             ConfigCallback config_change_callback,
             Logger* logger)
            : page_change_callback_(page_change_callback),
              config_change_callback_(config_change_callback),
              logger_(logger)
            {}
        virtual ~Page(){}

        virtual PB_SmartKnobConfig * getPageConfig() = 0;

        virtual void handleState(PB_SmartKnobState state) = 0;
        virtual void handleUserInput(input_t input, int input_data, PB_SmartKnobState state) = 0;

        // void setLogger(Logger* logger) {
        //     logger_ = logger;
        // }
        void log(const std::string& msg) {
            // if (logger_ == nullptr) assert(false); // Attempting to log without a logger
            logger_->log(msg);
        }
    
    protected:
        PageChangeCallback page_change_callback_;
        ConfigCallback config_change_callback_;

        Logger *logger_;
};
