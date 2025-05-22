#include "volume_page.h"

PB_SmartKnobConfig * VolumePage::getPageConfig() {
    return &config_;
}

// void VolumePage::checkForBrightnessUpdates(PB_SmartKnobState &state, PB_SmartKnobConfig &config) {
//     BrightnessData brightness_data;
//     if (xQueueReceive(incoming_volume_queue_, &brightness_data, 0) != pdTRUE) {
//         return; // No new brightness data, return
//     }

//     const uint8_t brightness = brightness_data.brightness;
//     int32_t new_position = volumeToPosition(brightness, config_);
//     LOG_INFO("LIGHTS: Received brightness from MQTT (%d) → position: %d", brightness, new_position);

//     const uint32_t time_since_last_update = millis() - last_publish_time_;
//     if (time_since_last_update < BRIGHTNESS_UPDATE_COOLDOWN_MS) {
//         LOG_WARN("LIGHTS: Ignoring brightness update due to cooldown (%lu ms)", time_since_last_update);
//         return;
//     }
    
//     PB_SmartKnobConfig* page_config = getPageConfig();
//     new_position = volumeToPosition(brightness, *page_config);
//     page_config->initial_position = new_position; // Set the initial position, so when the config is applied, the motor task will set this as the current position
//     config_change_callback_(page_config);
//     LOG_INFO("LIGHTS: Updating position to match received brightness");

//     state.current_position = new_position;
//     last_published_position_ = new_position;
// }

uint8_t positionToVolume(int32_t position, PB_SmartKnobConfig config) {
    float mapped = mapf(position, config.min_position, config.max_position, VOLUME_MIN, VOLUME_MAX); // in-min, in-max, out-min, out-max
    uint8_t volume = round(mapped);
    return volume;
}
int32_t volumeToPosition(uint8_t volume, PB_SmartKnobConfig config) {
    // Alert if volume is out of range
    if (volume < VOLUME_MIN || volume > VOLUME_MAX) {
        assert(false); // TODO: Handle this case, include logging
    }

    float mapped = mapf(volume, VOLUME_MIN, VOLUME_MAX, config.min_position, config.max_position); // in-min, in-max, out-min, out-max
    int32_t position = round(mapped);
    return position;
}

void VolumePage::handleState(PB_SmartKnobState state) {
    // Incoming
    // checkForBrightnessUpdates(state, config_);

    // Outgoing
    if (last_published_position_ != state.current_position) {
        if (millis() - last_publish_time_ > VOLUME_PUBLISH_FREQUENCY_MS) {
            LOG_INFO(
                "VOLUME: Publishing position to MQTT: %d (volume: %d)",
                state.current_position,
                positionToVolume(state.current_position, config_)
            );

            VolumeData msg = { .volume = positionToVolume(state.current_position, config_), };
            connectivity_task_.sendMqttMessage(msg);
            last_publish_time_ = millis();
            last_published_position_ = state.current_position;
        }
    }
}

void VolumePage::handleUserInput(input_t input, int input_data, PB_SmartKnobState state) {    
    switch (input)
    {
    case INPUT_BACK:
    case INPUT_FORWARD:
    {
        pageChange(PageType::MEDIA_MENU_PAGE);
        break;
    }
    default:
        break;
    }
}
