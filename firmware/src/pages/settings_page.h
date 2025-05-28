#pragma once

#include "page.h"
#include "views/view.h"

#include "tasks/motor_task.h"

enum class SettingsMenu {
    BACK = 0,
    CALIBRATE_MOTOR,
    // CALIBRATE_STRAIN
};

class SettingsPage: public Page {
    public:
        SettingsPage(PageContext& context) : Page(context) {}

        ~SettingsPage(){}

        PB_SmartKnobConfig * getPageConfig() override;
        void handleState(PB_SmartKnobState state) override {};
        void handleUserInput(input_t input, int input_data, PB_SmartKnobState state) override;
    
    private:
        void handleMenuInput(int position);

        PB_ViewConfig view_config = {
            VIEW_LIST_MENU,
            "Settings",
            .menu_entries_count = 2,
            .menu_entries = {
                { "Back",            ICON_BACK_ARROW },
                { "Calibrate motor", ""              },
                
                // This shouldn't be a menu option, since getting it wrong will cause the knob to be unusable
                // It should only be available remotely, from the terminal (or web interface/app?)
                // { "Calibrate strain", ""             },
            }
        };

        PB_SmartKnobConfig config_ =
        {
            .has_view_config = true,
            .view_config = view_config,
            .initial_position = 0,
            .sub_position_unit = 0,
            .position_nonce = 1,
            .min_position = 0,
            .max_position = 1,
            .infinite_scroll = false,
            .position_width_radians = 45 * PI / 180,
            .detent_strength_unit = 0.5,
            .endstop_strength_unit = 1,
            .snap_point = 0.51,
            .detent_positions_count = 0,
            .detent_positions = {},
            .snap_point_bias = 0,
            .led_hue = 200
        };
};
