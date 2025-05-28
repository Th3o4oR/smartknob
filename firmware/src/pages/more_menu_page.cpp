#include "more_menu_page.h"

PB_SmartKnobConfig * MorePage::getPageConfig() {
    return &config_;
}

void MorePage::handleMenuInput(int position) {
    auto menu_item = static_cast<MoreMenu>(position); // Convert the position to the corresponding menu item
    switch (menu_item) {
        case MoreMenu::BACK: {
            pageChange(PageType::MAIN_MENU_PAGE);
            break;
        }
        case MoreMenu::DEMO_CONFIGS: {
            pageChange(PageType::DEMO_PAGE);
            break;
        }
        default:
            break;
    }
}

void MorePage::handleUserInput(input_t input, int input_data, PB_SmartKnobState state) {    
    switch (input) {
        case INPUT_BACK: {
            pageChange(PageType::MAIN_MENU_PAGE);
            break;
        }
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
