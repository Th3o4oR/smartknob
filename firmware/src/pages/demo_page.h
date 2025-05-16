#pragma once

#include "page.h"
#include "views/view.h"
#include "interface_callbacks.h"

#include "tasks/motor_task.h"

class DemoPage : public Page {
    public:
        DemoPage(PageChangeCallback page_change_callback
               , ConfigCallback config_change_callback
               , Logger* logger)
            : Page(page_change_callback, config_change_callback, logger)
            {}

        ~DemoPage(){}

        PB_SmartKnobConfig * getPageConfig() override;
        void handleState(PB_SmartKnobState state) override {};
        void handleUserInput(input_t input, int input_data, PB_SmartKnobState state) override;
    
    private:
        int current_config_ = 0;

        PB_SmartKnobConfig configs_[11] =
        {
            {
                .has_view_config = true,
                .view_config = 
                {
                    VIEW_DIAL,
                    "Unbounded"
                },
                .initial_position = 0,
                .sub_position_unit = 0,
                .position_nonce = 0,
                .min_position = 0,
                .max_position = -1,
                .infinite_scroll = false,
                .position_width_radians = 10 * PI / 180,
                .detent_strength_unit = 0,
                .endstop_strength_unit = 1,
                .snap_point = 1.1,
                .detent_positions_count = 0,
                .detent_positions = {},
                .snap_point_bias = 0,
                .led_hue = 200
            },
            {
                .has_view_config = true,
                .view_config = 
                {
                    VIEW_DIAL,
                    "Bounded 0-10"
                },
                .initial_position = 0,
                .sub_position_unit = 0,
                .position_nonce = 1,
                .min_position = 0,
                .max_position = 10,
                .infinite_scroll = false,
                .position_width_radians = 10 * PI / 180,
                .detent_strength_unit = 0,
                .endstop_strength_unit = 1,
                .snap_point = 1.1,
                .detent_positions_count = 0,
                .detent_positions = {},
                .snap_point_bias = 0,
                .led_hue = 0
            },
            {
                .has_view_config = true,
                .view_config = 
                {
                    VIEW_DIAL,
                    "Multi-rev\nNo detents"
                },
                .initial_position = 0,
                .sub_position_unit = 0,
                .position_nonce = 2,
                .min_position = 0,
                .max_position = 72,
                .infinite_scroll = false,
                .position_width_radians = 10 * PI / 180,
                .detent_strength_unit = 0,
                .endstop_strength_unit = 1,
                .snap_point = 1.1,
                .detent_positions_count = 0,
                .detent_positions = {},
                .snap_point_bias = 0,
                .led_hue = 73
            },
            {
                .has_view_config = true,
                .view_config = 
                {
                    VIEW_DIAL,
                    "On/off\nStrong detent"
                },
                .initial_position = 0,
                .sub_position_unit = 0,
                .position_nonce = 3,
                .min_position = 0,
                .max_position = 1,
                .infinite_scroll = false,
                .position_width_radians = 60 * PI / 180,
                .detent_strength_unit = 1,
                .endstop_strength_unit = 1,
                .snap_point = 0.55, // Note the snap point is slightly past the midpoint (0.5); compare to normal detents which use a snap point *past* the next value (i.e. > 1)
                .detent_positions_count = 0,
                .detent_positions = {},
                .snap_point_bias = 0,
                .led_hue = 157
            },
            {
                .has_view_config = true,
                .view_config = 
                {
                    VIEW_DIAL,
                    "Return-to-center"
                },
                .initial_position = 0,
                .sub_position_unit = 0,
                .position_nonce = 4,
                .min_position = 0,
                .max_position = 0,
                .infinite_scroll = false,
                .position_width_radians = 60 * PI / 180,
                .detent_strength_unit = 0.01,
                .endstop_strength_unit = 0.6,
                .snap_point = 1.1,
                .detent_positions_count = 0,
                .detent_positions = {},
                .snap_point_bias = 0,
                .led_hue = 45
            },
            {
                .has_view_config = true,
                .view_config = 
                {
                    VIEW_DIAL,
                    "Fine values\nNo detents"
                },
                .initial_position = 127,
                .sub_position_unit = 0,
                .position_nonce = 5,
                .min_position = 0,
                .max_position = 255,
                .infinite_scroll = false,
                .position_width_radians = 1 * PI / 180,
                .detent_strength_unit = 0,
                .endstop_strength_unit = 1,
                .snap_point = 1.1,
                .detent_positions_count = 0,
                .detent_positions = {},
                .snap_point_bias = 0,
                .led_hue = 219
            },
            {
                .has_view_config = true,
                .view_config = 
                {
                    VIEW_DIAL,
                    "Fine values\nWith detents"
                },
                .initial_position = 127,
                .sub_position_unit = 0,
                .position_nonce = 5,
                .min_position = 0,
                .max_position = 255,
                .infinite_scroll = false,
                .position_width_radians = 1 * PI / 180,
                .detent_strength_unit = 1,
                .endstop_strength_unit = 1,
                .snap_point = 1.1,
                .detent_positions_count = 0,
                .detent_positions = {},
                .snap_point_bias = 0,
                .led_hue = 25
            },
            {
                .has_view_config = true,
                .view_config = 
                {
                    VIEW_DIAL,
                    "Coarse values\nStrong detents"
                },
                .initial_position = 0,
                .sub_position_unit = 0,
                .position_nonce = 6,
                .min_position = 0,
                .max_position = 31,
                .infinite_scroll = false,
                .position_width_radians = 8.225806452 * PI / 180,
                .detent_strength_unit = 2,
                .endstop_strength_unit = 1,
                .snap_point = 1.1,
                .detent_positions_count = 0,
                .detent_positions = {},
                .snap_point_bias = 0,
                .led_hue = 200
            },
            {
                .has_view_config = true,
                .view_config = 
                {
                    VIEW_DIAL,
                    "Coarse values\nWeak detents"
                },
                .initial_position = 0,
                .sub_position_unit = 0,
                .position_nonce = 6,
                .min_position = 0,
                .max_position = 31,
                .infinite_scroll = false,
                .position_width_radians = 8.225806452 * PI / 180,
                .detent_strength_unit = 0.2,
                .endstop_strength_unit = 1,
                .snap_point = 1.1,
                .detent_positions_count = 0,
                .detent_positions = {},
                .snap_point_bias = 0,
                .led_hue = 0
            },
            {
                .has_view_config = true,
                .view_config = 
                {
                    VIEW_DIAL,
                    "Magnetic detents"
                },
                .initial_position = 0,
                .sub_position_unit = 0,
                .position_nonce = 7,
                .min_position = 0,
                .max_position = 31,
                .infinite_scroll = false,
                .position_width_radians = 7 * PI / 180,
                .detent_strength_unit = 2.5,
                .endstop_strength_unit = 1,
                .snap_point = 0.7,
                .detent_positions_count = 4,
                .detent_positions = {2, 10, 21, 22},
                .snap_point_bias = 0,
                .led_hue = 73
            },
            {
                .has_view_config = true,
                .view_config = 
                {
                    VIEW_DIAL,
                    "Return-to-center\nwith detents"
                },
                .initial_position = 0,
                .sub_position_unit = 0,
                .position_nonce = 8,
                .min_position = -6,
                .max_position = 6,
                .infinite_scroll = false,
                .position_width_radians = 60 * PI / 180,
                .detent_strength_unit = 1,
                .endstop_strength_unit = 1,
                .snap_point = 0.55,
                .detent_positions_count = 0,
                .detent_positions = {},
                .snap_point_bias = 0.4,
                .led_hue = 157
            }
        };
};
