#include "page.h"
#include "tasks/motor_task.h"
#include "views/view.h"
#include "views/circle_menu_view.h"

enum class MoreMenu {
    BACK = 0,
    DEMO_CONFIGS,

    _MAX
};

class MorePage : public Page {
    public:
        MorePage() : Page() {}

        ~MorePage(){}

        PB_SmartKnobConfig * getPageConfig() override;
        void handleState(PB_SmartKnobState state) override {};
        void handleUserInput(input_t input, int input_data, PB_SmartKnobState state) override;
    
    private:
        void handleMenuInput(int position);

        PB_ViewConfig view_config = {
            VIEW_CIRCLE_MENU,
            "More",
            .menu_entries_count = 2,
            .menu_entries = {
                { "Back",          ICON_BACK_ARROW },
                { "Demo\nconfigs", ICON_NOTE_STACK },
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
            .max_position = static_cast<int>(MoreMenu::_MAX) - 1,
            .infinite_scroll = false,
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
