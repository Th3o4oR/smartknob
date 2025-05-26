#pragma once

#include <functional>

#include "../proto_gen/smartknob.pb.h"
#include "../input_type.h"

#include "logger.h"

typedef enum {
    MAIN_MENU_PAGE = 0,
    // LIGHTS_PAGE,
    SETTINGS_PAGE,
    MORE_PAGE,
    DEMO_PAGE,
    LIGHTS_PAGE_RED,
    LIGHTS_PAGE_YELLOW,
    LIGHTS_PAGE_GREEN,
    LIGHTS_PAGE_AQUA,
    LIGHTS_PAGE_BLUE,
    LIGHTS_PAGE_PURPLE,
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

        virtual void setVisited(bool visited) {
            visited_ = visited;
        }
        virtual bool getVisited() {
            return visited_;
        }
        virtual void setPreviousPosition(int32_t position) {
            previous_position_ = position;
        }
        virtual int32_t getPreviousPosition() {
            return previous_position_;
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
        bool visited_ = false; // Keeps track of whether the page has been visited before. This is used to determine whether to set the initial position to the previous position on the page
        int32_t previous_position_ = 0; // The previous position on the page, if it has been visited before

        PageChangeCallback page_change_callback_;

        Logger *logger_;
};
