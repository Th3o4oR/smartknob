#pragma once

#include <functional>

#include "../proto_gen/smartknob.pb.h"
#include "../input_type.h"

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
    
    protected:
        bool visited_ = false; // Keeps track of whether the page has been visited before. This is used to determine whether to set the initial position to the previous position on the page
        int32_t previous_position_ = 0; // The previous position on the page, if it has been visited before
        
        // Records the time when the page was selected.
        // This allows a page to implement a sort of "cooldown" from it is selected.
        // Necessary since the motor_task runs on a separate core, and periodically sends updates to the interface_task.
        //     However, when the user changes to a new menu page, the motor_task will still send updates for the old page until it receives the new config. 
        //     This can lead to the lights_page receiving the previous menu page's position, and treating this as the actual knob position.
        //     Example: The previous position on the lights page was 213, and the user moves from the main menu to this page (where the position of the lights page on the main menu is 0). 
        //         This might lead to the lights page incorrectly assuming that the knob has turned to position 0, when it was previously at 213, and send an MQTT message with this value.
        //         When the motor_task updates its config and sends a new value over the channels (5ms later as of writing this comment), the lights page will send a new MQTT message with the correct value (213).
        //     This variable could for example be used to prevent the lights page from sending MQTT messages within the first 20ms after each time the page is entered,
        //     giving the motor task enough time to catch up.
        // unsigned long page_selection_time_ = 0;

        PageChangeCallback page_change_callback_;
};
