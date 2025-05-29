#include "demo_page.h"

PB_SmartKnobConfig * DemoPage::getPageConfig() {
    return &configs_[0];
}

void DemoPage::handleUserInput(input_t input, int input_data, PB_SmartKnobState state) {
    switch (input)
    {
    case INPUT_BACK:
    {
        current_config_ = 0;
        pageChange(PageID::MORE);
        break;
    }
    case INPUT_NEXT:
    {
        current_config_++;
        if (current_config_ < sizeof(configs_) / sizeof(configs_[0])) {
            configChange(configs_[current_config_]);
        } else {
            current_config_ = 0;
            configChange(configs_[current_config_]);
        }
        break;
    }
    case INPUT_FORWARD: 
    {
        current_config_++;
        if (current_config_ < sizeof(configs_) / sizeof(configs_[0])) {
            configChange(configs_[current_config_]);
        } else {
            current_config_ = 0;
            pageChange(PageID::MORE);
        }
        break;
    }
    case INPUT_PREV:
    {
        current_config_--;
        if (current_config_ >= 0) {
            configChange(configs_[current_config_]);
        } else {
            current_config_ = sizeof(configs_) / sizeof(configs_[0]);
            configChange(configs_[current_config_]);
        }
        break;
    }    
    default:
        break;
    }
}
