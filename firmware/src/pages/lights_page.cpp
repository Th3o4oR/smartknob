#include "lights_page.h"

PB_SmartKnobConfig * LightsPage::getPageConfig() {
    return &config_;
}

void LightsPage::handleState(PB_SmartKnobState state) {
    // Prevent publishing of MQTT messages within a certain time after the page is selected
    if (millis() - page_selection_time_ < PAGE_SELECTION_COOLDOWN) {
        return;
    }

    if (last_published_position != state.current_position) {
        if (millis() - last_publish_time > 500) {
            int current_position = state.current_position;
            Message msg = {
                .trigger_name = "lights",
                .trigger_value = current_position
            };
            connectivity_task_.sendMqttMessage(msg);
            last_publish_time = millis();
            last_published_position = state.current_position;
        }
    }
}

void LightsPage::handleUserInput(input_t input, int input_data, PB_SmartKnobState state) {    
    switch (input)
    {
    case INPUT_BACK:
    {
        if (page_change_callback_) {
            page_change_callback_(MAIN_MENU_PAGE);
        }
        break;
    }
    case INPUT_FORWARD:
    {
        if (page_change_callback_) {
            page_change_callback_(MAIN_MENU_PAGE);
        }
        break;
    }    
    default:
        break;
    }
}
