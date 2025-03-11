
#include "page.h"
#include "interface_callbacks.h"
#include "../motor_task.h"
#include "views/view.h"

class SettingsPage: public Page {
    public:
        SettingsPage(MotorCalibrationCallback motor_calibration_callback) : Page(), motor_calibration_callback_(motor_calibration_callback) {}

        ~SettingsPage(){}

        PB_SmartKnobConfig * getPageConfig() override;
        void handleState(PB_SmartKnobState state) override {};
        void handleUserInput(input_t input, int input_data, PB_SmartKnobState state) override;
    
    private:
        MotorCalibrationCallback motor_calibration_callback_;
        
        void handleMenuInput(int position);
        
        #if SK_STRAIN
            int maxPositions = 2;
        #else
            int maxPositions = 1;
        #endif

        PB_ViewConfig view_config = {
            VIEW_LIST_MENU,
            "Settings",
            .menu_entries_count = 3,
            .menu_entries = 
            {
                {
                    "Back",
                    "\ue5c4"
                },
                {
                    "Calibrate motor",
                    ""
                },
                {
                    "Calibrate strain",
                    ""
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
            .max_position = maxPositions,
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
