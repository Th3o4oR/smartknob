#include "lights_page.h"

PB_SmartKnobConfig * LightsPage::getPageConfig() {
    return &config_;
}


uint8_t positionToBrightness(int32_t position, PB_SmartKnobConfig config) {
    float mapped = mapf(position, config.min_position, config.max_position, 0, 255); // in-min, in-max, out-min, out-max
    uint8_t brightness = round(mapped);
    return brightness;
}
int32_t brightnessToPosition(uint8_t brightness, PB_SmartKnobConfig config) {
    float mapped = mapf(brightness, 0, 255, config.min_position, config.max_position); // in-min, in-max, out-min, out-max
    int32_t position = round(mapped);
    return position;
}

void LightsPage::checkForBrightnessUpdates(PB_SmartKnobState &state, PB_SmartKnobConfig &config) {
    BrightnessData brightness_data;
    if (xQueueReceive(incoming_brightness_queue_, &brightness_data, 0) != pdTRUE) {
        return; // No new brightness data, return
    }

    const uint8_t brightness = brightness_data.brightness;
    int32_t new_position = brightnessToPosition(brightness, config_);
    LOG_INFO("LIGHTS: Received brightness from MQTT (%d) → position: %d", brightness, new_position);

    const uint32_t time_since_last_update = millis() - last_publish_time_;
    if (time_since_last_update < BRIGHTNESS_UPDATE_COOLDOWN_MS) {
        LOG_WARN("LIGHTS: Ignoring brightness update due to cooldown (%lu ms)", time_since_last_update);
        return;
    }
    
    PB_SmartKnobConfig* page_config = getPageConfig();
    new_position = brightnessToPosition(brightness, *page_config);
    page_config->initial_position = new_position; // Set the initial position, so when the config is applied, the motor task will set this as the current position
    config_change_callback_(*page_config);
    LOG_INFO("LIGHTS: Updating position to match received brightness");

    state.current_position = new_position;
    last_published_position_ = new_position;
}

void LightsPage::handleState(PB_SmartKnobState state) {
    // Incoming
    checkForBrightnessUpdates(state, config_);

    // Outgoing
    if (last_published_position_ != state.current_position) {
        if (millis() - last_publish_time_ > MQTT_PUBLISH_FREQUENCY_MS) {
            LOG_INFO(
                "LIGHTS: Publishing position to MQTT: %d (brightness: %d)",
                state.current_position,
                positionToBrightness(state.current_position, config_)
            );
            BrightnessData msg = { .brightness = positionToBrightness(state.current_position, config_), };
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
        page_change_callback_(PageType::MAIN_MENU_PAGE);
        break;
    }
    case INPUT_FORWARD:
    {
        page_change_callback_(PageType::MAIN_MENU_PAGE);
        break;
    }    
    default:
        break;
    }
}
