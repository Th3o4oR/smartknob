#include "settings_page.h"

PB_SmartKnobConfig * SettingsPage::getPageConfig() {
    return &config_;
}

void SettingsPage::handleMenuInput(int position) {
    switch (position)
    {
    case SettingsMenuEntries::BACK:
    {
        if (page_change_callback_) {
            page_change_callback_(MAIN_MENU_PAGE);
        }
        break;
    }
    case SettingsMenuEntries::CALIBRATE_MOTOR:
    {
        motor_calibration_callback_();
        break;
    }
    
    default:
        break;
    }
}

void SettingsPage::handleUserInput(input_t input, int input_data, PB_SmartKnobState state) {    
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
        int current_position = state.current_position;
        handleMenuInput(current_position);
        break;
    }    
    case INPUT_WITH_DATA:
    {
        handleMenuInput(input_data);
        break;
    }
    default:
        break;
    }
}
