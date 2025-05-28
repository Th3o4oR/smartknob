#include "volume_page.h"

PB_SmartKnobConfig * VolumePage::getPageConfig() {
    return &config_;
}

void VolumePage::handleState(PB_SmartKnobState state) {
    // Incoming
    // checkForVolumeUpdates(state, config_);

    // Outgoing
    if (last_published_position_ != state.current_position) {
        if (millis() - last_publish_time_ > VOLUME_PUBLISH_FREQUENCY_MS) {
            VolumeData msg = { .volume = remap<float>(state.current_position, config_.min_position, config_.max_position, VOLUME_MIN, VOLUME_MAX) };
            // LOG_INFO("VOLUME: Publishing position to MQTT: %d (volume: %f)", state.current_position, msg.volume);
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
