#include "media_control_page.h"

PB_SmartKnobConfig *MediaMenuPage::getPageConfig() {
    return &config_;
}

void MediaMenuPage::handleState(PB_SmartKnobState state) {
}

void MediaMenuPage::handleMenuInput(int position) {
    auto menu_item = static_cast<MediaMenu>(position); // Convert the position to the corresponding menu item
    switch (menu_item) {
    case MediaMenu::BACK: {
        page_change_callback_(MAIN_MENU_PAGE);
        break;
    }
    case MediaMenu::PLAY_PAUSE: {
        paused_            = !paused_;
        PlayPauseData data = {.paused = paused_};
        LOG_INFO(
            "MEDIA: Sending play/pause command to MQTT (%s)",
            paused_ ? "pause" : "play"
        );
        connectivity_task_.sendMqttMessage(data);
        break;
    }
    case MediaMenu::SKIP_PREV: {
        // Skip to previous track
        SkipData data = {.forward = false};
        LOG_INFO("MEDIA: Sending skip previous command to MQTT");
        connectivity_task_.sendMqttMessage(data);
        break;
    }
    case MediaMenu::SKIP_NEXT: {
        // Skip to next track
        SkipData data = {.forward = true};
        LOG_INFO("MEDIA: Sending skip next command to MQTT");
        connectivity_task_.sendMqttMessage(data);
        break;
    }
    case MediaMenu::VOLUME: {
        page_change_callback_(VOLUME_PAGE);
        break;
    }
    default:
        break;
    }
}

void MediaMenuPage::handleUserInput(input_t input, int input_data, PB_SmartKnobState state) {
    switch (input) {
    case INPUT_FORWARD: {
        int current_position = state.current_position;
        handleMenuInput(current_position);
        break;
    }
    case INPUT_WITH_DATA: {
        handleMenuInput(input_data);
        break;
    }
    default:
        break;
    }
}
