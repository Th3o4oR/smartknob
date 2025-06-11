#pragma once

#include "view.h"
#include "tasks/display_task.h"
#include "proto_gen/smartknob.pb.h"

class CircleMenuView: public View {
    public:
        static const int MAX_BUTTONS = 8;
        static constexpr const float DETENT_STRENGTH_UNIT = 0.5f;
        static constexpr const float ENDSTOP_STRENGTH_UNIT = 1.0f;
        static constexpr const float SNAP_POINT = 0.51f;
        
        CircleMenuView(lv_obj_t * screen, DisplayTask* display_task) {
            display_task_ = display_task;
            screen_ = screen;
        }
        ~CircleMenuView() {}


        void setupView(PB_SmartKnobConfig config) override;
        void updateView(PB_SmartKnobState state) override;

    private:
        DisplayTask* display_task_;
        lv_obj_t *screen_;
        lv_obj_t *label_desc;
        lv_obj_t *arc;
        lv_obj_t *arc_dot;
        lv_obj_t *buttons[MAX_BUTTONS];
        void setup_circle_menu_buttons(PB_SmartKnobConfig config);
        void setup_menu_elements();
};
