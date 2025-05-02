#pragma once

#include <functional>

#include "../proto_gen/smartknob.pb.h"
#include "../input_type.h"

#include "logger.h"

typedef enum {
    MAIN_MENU_PAGE = 0,
    LIGHTS_PAGE,
    SETTINGS_PAGE,
    MORE_PAGE,
    DEMO_PAGE,
} page_t;

/**
 * @brief Callback to handle page changes
 * 
 * @param page The requested page
 */
typedef std::function<void(page_t)> PageChangeCallback;

class Page {
    public:
        Page() {}
        virtual ~Page(){}

        virtual PB_SmartKnobConfig * getPageConfig() = 0;

        virtual void handleState(PB_SmartKnobState state) = 0;
        virtual void handleUserInput(input_t input, int input_data, PB_SmartKnobState state) = 0;
        virtual void setPageChangeCallback(PageChangeCallback cb) {
            page_change_callback_ = cb;
        }

        void setLogger(Logger* logger) {
            logger_ = logger;
        }
        void log(const std::string& msg) {
            if (logger_ != nullptr) {
                logger_->log(msg);
            }
        }
    
    protected:
        PageChangeCallback page_change_callback_;

        Logger *logger_;
};
