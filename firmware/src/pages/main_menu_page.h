#pragma once

#include "page.h"
#include "views/view.h"
#include "../motor_task.h"

class MainMenuPage : public Page {
    public:
        MainMenuPage() : Page() {}

        ~MainMenuPage(){}

        PB_SmartKnobConfig * getPageConfig() override;
        void handleState(PB_SmartKnobState state) override {};
        void handleUserInput(input_t input, int input_data, PB_SmartKnobState state) override;
    
    private:
        void handleMenuInput(int position);  

        PB_ViewConfig view_config = {
            VIEW_CIRCLE_MENU,
            "Main menu",
            .menu_entries_count = 8,
            .menu_entries = 
            {
                {
                    "Desk\nlights",
                    "\ue0f0"
                },
                {
                    "Room\nlights",
                    "\uf02a"
                },
                {
                    "Shades",
                    "\uec12"
                },
                {
                    "Heating",
                    "\ue1ff"
                },
                {
                    "Media",
                    "\uf137"
                },
                {
                    "Timer",
                    "\ue425"
                },
                {
                    "Settings",
                    "\ue8b8"
                },
                {
                    "More",
                    "\ue5d3"
                }
            }
        };

        PB_SmartKnobConfig config_ =
        {
            .has_view_config = true,
            .view_config = view_config,
            .initial_position = 0,
            .sub_position_unit = 0,
            .position_nonce = 0,
            .min_position = 0,
            .max_position = 7,
            .infinite_scroll = true,
            .position_width_radians = 45 * PI / 180,
            .detent_strength_unit = 0.5,
            .endstop_strength_unit = 1,
            .snap_point = 0.51,
            .detent_positions_count = 0,
            .detent_positions = {},
            .snap_point_bias = 0,
            .led_hue = 30
        };
};
