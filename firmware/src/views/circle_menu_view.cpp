#include <Arduino.h>

#include "circle_menu_view.h"
#include "view_util.h"
#include "styles.h"

void CircleMenuView::setup_circle_menu_buttons(PB_SmartKnobConfig config) {
    for (int i = 0; i < config.max_position + 1; i++) {
        setup_circle_button(&buttons[i], i, config, display_task_->button_event_cb);
    }
}

void CircleMenuView::setup_menu_elements() {
    lv_obj_align(label_desc, LV_ALIGN_CENTER, 0, MENU_DESCRIPTION_Y_OFFSET);
}

void CircleMenuView::setupView(PB_SmartKnobConfig config) {
    // setup_circle_elements(config);
    setup_circle_elements(&label_desc, &arc_dot, &arc);
    setup_circle_menu_buttons(config);
    setup_menu_elements();
}

void CircleMenuView::updateView(PB_SmartKnobState state) {
    if (lv_obj_is_valid(buttons[0]) && state.current_position >= 0 && state.current_position <= state.config.max_position) {
        for (int i = 0; i < state.config.max_position + 1; i++) {
            lv_obj_clear_state(buttons[i], LV_STATE_FOCUSED);
        }
        lv_obj_add_state(buttons[state.current_position], LV_STATE_FOCUSED);
    }

    lv_label_set_text(label_desc, state.config.view_config.menu_entries[state.current_position].description);
    lv_obj_set_style_text_align(label_desc, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(label_desc, LV_ALIGN_CENTER, 0, MENU_DESCRIPTION_Y_OFFSET); // Realigns label and its content to the center - this is required every time after setting text value

    lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN);

    float left_bound            = calculate_start_angle(state.config);
    float adjusted_sub_position = state.sub_position_unit * state.config.position_width_radians;
    float raw_angle             = left_bound - (state.current_position - state.config.min_position) * state.config.position_width_radians;
    float adjusted_angle        = raw_angle - adjusted_sub_position;

    int32_t num_positions = state.config.max_position - state.config.min_position + 1;
    if (num_positions > 0 && state.config.infinite_scroll == 0 && ((state.current_position == state.config.min_position && state.sub_position_unit < 0) || (state.current_position == state.config.max_position && state.sub_position_unit > 0))) {
        arc_dot_set_angle(arc_dot, raw_angle, arc_dot_menu_padding, arc_dot_size);
    } else {
        arc_dot_set_angle(arc_dot, adjusted_angle, arc_dot_menu_padding, arc_dot_size);
    }
}
