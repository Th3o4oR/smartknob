#include "page.h"
#include "../motor_task.h"
#include "views/view.h"

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
            .menu_entries = 
            {
                {
                    "Back",
                    "\ue5c4"
                },
                {
                    "Demo\nconfigs",
                    "\uf562"
                }
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
            .led_hue = 30
        };
};
