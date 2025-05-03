#pragma once

#include "page.h"
#include "views/view.h"
#include "views/circle_menu_view.h"
#include "tasks/motor_task.h"

// Needs to match the entries defined below
enum class MainMenu {
    SETTINGS,
    MORE,
    BEDROOM_LIGHTS,
    MEDIA,
    TIMER,
    DESK_LIGHTS,
    SHADES,
    HEATING,

    _MAX
};

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
            .view_type = VIEW_CIRCLE_MENU,
            "Main menu",
            .menu_entries_count = static_cast<int>(MainMenu::_MAX),
            .menu_entries = {
                { "Settings",        ICON_GEAR         },
                { "More",            ICON_ELLIPSIS     },
                { "Bedroom\nlights", ICON_CEILING_LAMP },
                { "Media",           ICON_PLAY_PAUSE   },
                { "Timer",           ICON_TIMER        },
                { "Desk\nlights",    ICON_CEILING_LAMP },
                { "Shades",          ICON_SHADES       },
                { "Heating",         ICON_HEATING      },
            }
        };

        PB_SmartKnobConfig config_ =
        {
            .has_view_config = true,
            .view_config = view_config,
            .initial_position = 2,
            .sub_position_unit = 0,
            .position_nonce = 0,
            .min_position = 0,
            .max_position = static_cast<int>(MainMenu::_MAX) - 1,
            .infinite_scroll = true,
            .position_width_radians = 2 * PI / CircleMenuView::MAX_BUTTONS,
            .detent_strength_unit = 0.5,
            .endstop_strength_unit = 1,
            .snap_point = 0.51,
            .detent_positions_count = 0,
            .detent_positions = {},
            .snap_point_bias = 0,
            .led_hue = 30
        };
};
