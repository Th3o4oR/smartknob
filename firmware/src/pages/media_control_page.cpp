#include "media_control_page.h"

PB_SmartKnobConfig * MediaMenuPage::getPageConfig() {
    return &config_;
}

void MediaMenuPage::handleMenuInput(int position) {
    auto menu_item = static_cast<MediaMenu>(position); // Convert the position to the corresponding menu item
    switch (menu_item) {
        case MediaMenu::BACK: {
            if (page_change_callback_) {
                page_change_callback_(MAIN_MENU_PAGE);
            }
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
