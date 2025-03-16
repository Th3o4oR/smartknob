#include "lights_page.h"

PB_SmartKnobConfig * LightsPage::getPageConfig() {
    return &config_;
}

uint8_t positionToBrightness(int32_t position, PB_SmartKnobConfig config) {
    return map(position, config.min_position, config.max_position, 0, 255); // in-min, in-max, out-min, out-max
}
int32_t brightnessToPosition(uint8_t brightness, PB_SmartKnobConfig config) {
    return map(brightness, 0, 255, config.min_position, config.max_position); // in-min, in-max, out-min, out-max
}

void LightsPage::handleState(PB_SmartKnobState state) {
    // Prevent publishing of MQTT messages within a certain time after the page is selected
    if (millis() - page_selection_time_ < PAGE_SELECTION_COOLDOWN) {
        return;
    }

    if (last_published_position_ != state.current_position) {
        if (millis() - last_publish_time_ > MQTT_PUBLISH_FREQUENCY_MS) {
            log(("LIGHTS: Publishing position to MQTT: " + String(state.current_position) + " (brightness: " + String(positionToBrightness(state.current_position, config_)) + ")").c_str());
            Message msg = {
                .trigger_name = "lights",
                .trigger_value = positionToBrightness(state.current_position, config_)
            };
            connectivity_task_.sendMqttMessage(msg);
            last_publish_time_ = millis();
            last_published_position_ = state.current_position;
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

void LightsPage::setLogger(Logger *logger) {
    logger_ = logger;
}

void LightsPage::log(const char *msg) {
    if (logger_ != nullptr) {
        logger_->log(msg);
    }
}
