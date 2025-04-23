#pragma once

#include "view.h"
#include "tasks/display_task.h"
#include "proto_gen/smartknob.pb.h"

class DialView: public View {
    public:
        DialView(lv_obj_t * screen, DisplayTask* display_task) {
            display_task_ = display_task;
            screen_ = screen;
        }
        ~DialView() {}

        void setupView(PB_SmartKnobConfig config) override;
        void updateView(PB_SmartKnobState state) override;

    private:
        PB_SmartKnobState previous_state_;

        DisplayTask* display_task_;
        lv_obj_t * screen_;
        lv_obj_t * label_cur_pos;
        lv_obj_t * label_desc;
        lv_obj_t * arc;
        lv_obj_t * arc_dot;
        lv_obj_t * line_left_bound;
        lv_obj_t * line_right_bound;
        void setup_dial_elements(PB_SmartKnobConfig config);
};
