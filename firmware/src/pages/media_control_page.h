#pragma once

#include "page.h"
#include "views/view.h"
#include "views/circle_menu_view.h"
#include "tasks/motor_task.h"
#include "tasks/connectivity_task.h"

// Needs to match the entries defined below
enum class MediaMenu {
    BACK = 0,
    SKIP_PREV,
    PLAY_PAUSE,
    SKIP_NEXT,
    VOLUME,
    // SOURCE,

    _MAX
};

class MediaMenuPage : public Page {
    public:
        MediaMenuPage(ConnectivityTask &connectivity_task)
            : Page()
            , connectivity_task_(connectivity_task)
            {}
        ~MediaMenuPage() {}

        PB_SmartKnobConfig * getPageConfig() override;
        void handleState(PB_SmartKnobState state) override;
        void handleUserInput(input_t input, int input_data, PB_SmartKnobState state) override;
    
    private:
        void handleMenuInput(int position);  

        ConnectivityTask& connectivity_task_;

        bool paused_ = false;

        PB_ViewConfig view_config = {
            .view_type = VIEW_CIRCLE_MENU,
            "Media Control", // Not allowed to use .description to initialize the string, see https://stackoverflow.com/a/70173056
            .menu_entries_count = static_cast<int>(MediaMenu::_MAX),
            .menu_entries = {
                { "Back",       ICON_BACK_ARROW },
                { "Previous",   ICON_SKIP_PREV  },
                { "Play/Pause", ICON_PLAY_PAUSE },
                { "Next",       ICON_SKIP_NEXT  },
                { "Volume",     ICON_VOLUME     },
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
            .max_position = static_cast<int>(MediaMenu::_MAX) - 1,
            .infinite_scroll = true,
            .position_width_radians = 2 * PI / CircleMenuView::MAX_BUTTONS,
            .detent_strength_unit = CircleMenuView::DETENT_STRENGTH_UNIT,
            .endstop_strength_unit = CircleMenuView::ENDSTOP_STRENGTH_UNIT,
            .snap_point = CircleMenuView::SNAP_POINT,
            .detent_positions_count = 0,
            .detent_positions = {},
            .snap_point_bias = 0,
            .led_hue = 30
        };
};
