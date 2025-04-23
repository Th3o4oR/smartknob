#pragma once

#include "view.h"
#include "tasks/display_task.h"
#include "proto_gen/smartknob.pb.h"

class ListMenuView: public View {
    public:
        ListMenuView(lv_obj_t * screen, DisplayTask* display_task) {
            display_task_ = display_task;
            screen_ = screen;
        }
        ~ListMenuView() {}

        void setupView(PB_SmartKnobConfig config) override;
        void updateView(PB_SmartKnobState state) override;

    private:
        DisplayTask* display_task_;
        lv_obj_t * screen_;
        lv_obj_t * list;
        lv_obj_t * arc_dot;
        lv_obj_t * button1;
        lv_obj_t * button2;
        lv_obj_t * button3;
        lv_obj_t * button4;
        lv_obj_t * button5;
        lv_obj_t * button6;
        lv_obj_t * button7;
        lv_obj_t * button8;
        lv_obj_t * buttons[8] = {button1, button2, button3, button4, button5, button6, button7, button8};
        void setup_list_elements(PB_SmartKnobConfig config);
        void setup_list_button(lv_obj_t ** button, int index, PB_SmartKnobConfig config);
};
