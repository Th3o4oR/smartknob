#include "lights_page.h"

PB_SmartKnobConfig * LightsPage::getPageConfig() {
    return &config_;
}

double mapf(double x, double in_min, double in_max, double out_min, double out_max) {
    const double run = in_max - in_min;
    assert(in_min < in_max);
    // if (run == 0) {
    //     return -1; // Avoid division by zero
    // }
    const double rise = out_max - out_min;
    const double delta = x - in_min;
    return (delta * rise) / run + out_min;
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

void LightsPage::checkForLightingUpdates(PB_SmartKnobState &state, PB_SmartKnobConfig &config) {
    LightingPayload lighting;
    if (xQueueReceive(incoming_lighting_queue_, &lighting, 0) != pdTRUE) {
        return; // No new lighting data, return
    }

    if (!std::holds_alternative<BrightnessData>(lighting)) {
        LOG_WARN("LIGHTS: Ignoring non-brightness data");
        return;
    }

    const auto& brightness_data = std::get<BrightnessData>(lighting);
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
    config_change_callback_(page_config);
    LOG_INFO("LIGHTS: Updating position to match received brightness");

    state.current_position = new_position;
    last_published_position_ = new_position;
}

void LightsPage::handleState(PB_SmartKnobState state) {
    // Incoming
    checkForLightingUpdates(state, config_);

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
