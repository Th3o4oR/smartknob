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
    // Prevent updating the brightness value from MQTT messages within a certain time after the last publish
    uint8_t brightness;
    if (xQueueReceive(brightness_queue_, &brightness, 0) == pdTRUE) {
        log(("LIGHTS: Received brightness from MQTT (" + String(brightness) + ")" + " (position: " + String(brightnessToPosition(brightness, config_)) + ")").c_str());
        if (millis() - last_publish_time_ < BRIGHTNESS_UPDATE_COOLDOWN_MS) { // TODO: This shouldn't use the last_publish_time_, but rather the time since the state was last updated (knob rotated)
            log("Ignoring brightness update due to cooldown");
        } else {
            log("Updating position to match received brightness");
            PB_SmartKnobConfig *page_config = getPageConfig();
            int32_t position = brightnessToPosition(brightness, *page_config);
            page_config->initial_position = position;
            config_change_callback_(page_config);
            state.current_position = position; // Update local variable for use in the rest of the function
            last_published_position_ = position; // Update the last published position to prevent immediate republishing
        }
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
