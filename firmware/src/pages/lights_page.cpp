#include "lights_page.h"

PB_SmartKnobConfig * LightsPage::getPageConfig() {
    return &config_;
}

void LightsPage::checkForBrightnessUpdates(PB_SmartKnobState &state, PB_SmartKnobConfig &config) {
    BrightnessData brightness_data;
    if (xQueueReceive(incoming_brightness_queue_, &brightness_data, 0) != pdTRUE) {
        return; // No new brightness data, return
    }

    const uint8_t brightness = brightness_data.brightness;
    const int32_t new_position = remap<float>(brightness, BRIGHTNESS_MIN, BRIGHTNESS_MAX, config.min_position, config.max_position);
    LOG_INFO("LIGHTS: Received brightness from MQTT (%d) → position: %d", brightness, new_position);

    const uint32_t time_since_last_update = millis() - last_publish_time_;
    if (time_since_last_update < BRIGHTNESS_UPDATE_COOLDOWN_MS) {
        LOG_WARN("LIGHTS: Ignoring brightness update due to cooldown (%lu ms)", time_since_last_update);
        return;
    }
    
    config_.initial_position = new_position; // Set the initial position, so when the config is applied, the motor task will set this as the current position
    configChange(config_);
    LOG_INFO("LIGHTS: Updating position to match received brightness");
    
    state.current_position = new_position;
    last_published_position_ = new_position;
}

void LightsPage::handleState(PB_SmartKnobState state) {
    // Incoming
    checkForBrightnessUpdates(state, config_);

    // Outgoing
    if (last_published_position_ != state.current_position) {
        if (millis() - last_publish_time_ > BRIGHTNESS_PUBLISH_FREQUENCY_MS) {
            BrightnessData msg = { .brightness = remap<float>(state.current_position, config_.min_position, config_.max_position, BRIGHTNESS_MIN, BRIGHTNESS_MAX) };
            // LOG_INFO("LIGHTS: Publishing position to MQTT: %d (brightness: %d)", state.current_position, msg.brightness);
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
    case INPUT_FORWARD:
    {
        pageChange(PageType::MAIN_MENU_PAGE);
        break;
    }
    default:
        break;
    }
}
