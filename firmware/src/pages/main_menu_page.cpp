#include "main_menu_page.h"

PB_SmartKnobConfig * MainMenuPage::getPageConfig() {
    return &config_;
}

void MainMenuPage::handleMenuInput(int position) {
    auto menu_item = static_cast<MainMenu>(position); // Convert the position to the corresponding menu item
    switch (menu_item) {
        case MainMenu::BEDROOM_LIGHTS: {
            pageChange(PageType::LIGHTS_PAGE);
            break;
        }
        case MainMenu::SETTINGS: {
            pageChange(PageType::SETTINGS_PAGE);
            break;
        }
        case MainMenu::MORE: {
            pageChange(PageType::MORE_PAGE);
            break;
        }
        case MainMenu::MEDIA: {
            pageChange(PageType::MEDIA_MENU_PAGE);
            break;
        }
        default:
            break;
    }
}

void MainMenuPage::handleUserInput(input_t input, int input_data, PB_SmartKnobState state) {
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
