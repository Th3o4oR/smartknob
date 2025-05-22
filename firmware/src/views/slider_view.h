#pragma once

#include "view.h"
#include "view_util.h"
#include "util.h"
#include "tasks/display_task.h"
#include "proto_gen/smartknob.pb.h"

#define ARC_WIDTH    15 // Need to be odd to center the arc dot
#define ARC_PADDING  10
#define ARC_DIAMETER (2 * RADIUS - ARC_PADDING - ARC_WIDTH/2)

#define ARC_DOT_PADDING (ARC_PADDING + ARC_WIDTH/2 - 1) // 1 pixel has been subtracted to account for flooring in ARC_WIDTH/2
// #define ARC_DOT_PADDING ARC_PADDING + 25 // 1 pixel has been subtracted to account for flooring in ARC_WIDTH/2
#define ARC_DOT_DIAMETER (ARC_WIDTH + 8)
#define ARC_DOT_OUTLINE 6

#define SLIDER_COUNTER_POSITION(angle_rad) (int)(ARC_DIAMETER/2 * -sinf(angle_rad))

// constexpr uint8_t ARC_WARNING_START = ARC_VALUES * 0.5;
// constexpr uint8_t ARC_DANGER_START  = ARC_VALUES * 0.85;
// constexpr uint8_t ARC_DANGER_END    = ARC_VALUES;

class SliderView: public View {
    public:
        SliderView(lv_obj_t * screen, DisplayTask* display_task) {
            display_task_ = display_task;
            screen_ = screen;
        }
        ~SliderView() {}

        void setupView(PB_SmartKnobConfig config) override;
        void updateView(PB_SmartKnobState state) override;

    private:
        // PB_SmartKnobState previous_state_;

        DisplayTask* display_task_;
        lv_obj_t * screen_;
        lv_obj_t * label_desc;
        lv_obj_t * arc_dot;
        lv_obj_t * arc;
        
        lv_obj_t * slider_arc;
        lv_obj_t * slider_counter_label;
        lv_obj_t * slider_icon;
        void setup_slider_elements(PB_SmartKnobConfig config);
};
