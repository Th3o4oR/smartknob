#include "main_menu_page.h"

PB_SmartKnobConfig * MainMenuPage::getPageConfig() {
    return &config_;
}

void MainMenuPage::handleMenuInput(int position) {
    switch (position)
    {
    case 0:
        if (page_change_callback_) {
            page_change_callback_(LIGHTS_PAGE_RED);
        }
        break;
    case 1:
        if (page_change_callback_) {
            page_change_callback_(LIGHTS_PAGE_YELLOW);
        }
        break;
    case 2:
        if (page_change_callback_) {
            page_change_callback_(LIGHTS_PAGE_GREEN);
        }
        break;
    case 3:
        if (page_change_callback_) {
            page_change_callback_(LIGHTS_PAGE_AQUA);
        }
        break;
    case 4:
        if (page_change_callback_) {
            page_change_callback_(LIGHTS_PAGE_BLUE);
        }
        break;
    case 5:
        if (page_change_callback_) {
            page_change_callback_(LIGHTS_PAGE_PURPLE);
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
