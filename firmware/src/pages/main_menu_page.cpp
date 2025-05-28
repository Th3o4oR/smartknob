#include "main_menu_page.h"

PB_SmartKnobConfig * MainMenuPage::getPageConfig() {
    return &config_;
}

void MainMenuPage::handleMenuInput(int position) {
    switch (position)
    {
    case 0:
        if (page_change_callback_) {
            page_change_callback_(LIGHTS_PAGE);
        }
        break;
    case 6:
        if (page_change_callback_) {
            page_change_callback_(SETTINGS_PAGE);
        }
        break;
    case 7:
        if (page_change_callback_) {
            page_change_callback_(MORE_PAGE);
        }
        break;
    
    default:
        break;
    }
}

void MainMenuPage::handleUserInput(input_t input, int input_data, PB_SmartKnobState state) {
    switch (input)
    {
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