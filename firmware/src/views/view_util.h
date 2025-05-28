#pragma once

#include "lvgl.h"
#include "proto_gen/smartknob.pb.h"

const int RADIUS = TFT_WIDTH / 2;

void arc_dot_set_angle(lv_obj_t * arc_dot, float angle, int padding, int dot_size);
void set_screen_gradient(int value);
void setup_circle_elements(lv_obj_t ** label_desc, lv_obj_t ** arc_dot, lv_obj_t ** arc);
void setup_circle_button(lv_obj_t ** button, int index, PB_SmartKnobConfig config, lv_event_cb_t button_event_cb);
