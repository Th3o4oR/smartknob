#include "slider_view.h"
#include "styles.h"

/**
 * @brief Update the text content of a label
 * @note Realigns label and its content to the center - this is required every time after setting text value
 * 
 * @param label The label to update
 * @param text The text to set
 * @param x_offset The x offset to use
 * @param y_offset The y offset to use
 */
void update_label(lv_obj_t *label, const char *text, int x_offset, int y_offset) {
    lv_label_set_text(label, text);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, x_offset, y_offset);
}

void SliderView::setup_slider_elements(const PB_SmartKnobConfig config) {
    float range_radians   = (config.max_position - config.min_position) * config.position_width_radians;
    float range_degrees   = range_radians * (180 / PI);
    uint16_t left_bound_deg  = roundf(270 - range_degrees / 2); // 270 degrees (up) - half of range in degrees
    uint16_t right_bound_deg = roundf(270 + range_degrees / 2); // 270 degrees (up) + half of range in degrees
    
    volume_arc = lv_arc_create(screen_);
    lv_obj_center(volume_arc);
    lv_obj_set_size(volume_arc, ARC_DIAMETER, ARC_DIAMETER);
    lv_arc_set_angles(volume_arc, left_bound_deg, right_bound_deg); // Remember that LVGL uses degrees, going clockwise from the right
    lv_arc_set_bg_angles(volume_arc, left_bound_deg, right_bound_deg);

    static lv_style_t arc_style_main, arc_style_indicator;
    lv_style_init(&arc_style_main);
    lv_style_set_arc_color(&arc_style_main, lv_palette_darken(LV_PALETTE_GREY, 1));
    lv_style_set_arc_width(&arc_style_main, ARC_WIDTH);
    lv_style_set_bg_opa(&arc_style_main, LV_OPA_TRANSP);

    lv_style_init(&arc_style_indicator);
    lv_style_set_arc_width(&arc_style_indicator, ARC_WIDTH);
    lv_style_set_arc_opa(&arc_style_indicator, LV_OPA_TRANSP);

    lv_obj_add_style(volume_arc, &arc_style_main, LV_PART_MAIN);
    lv_obj_add_style(volume_arc, &arc_style_indicator, LV_PART_INDICATOR);
    lv_obj_remove_style(volume_arc, NULL, LV_PART_KNOB);

    volume_icon = lv_label_create(screen_);
    lv_obj_align(volume_icon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(volume_icon, &roboto_light_60, 0);
    lv_obj_set_style_text_color(volume_icon, lv_color_white(), 0);
    lv_obj_set_style_text_align(volume_icon, LV_ALIGN_CENTER, LV_PART_MAIN);
    
    volume_label = lv_label_create(screen_);
    lv_obj_align(volume_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(volume_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(volume_label, &roboto_light_24, 0);
    
    // if (!config.has_view_config) {
    //     LOG_ERROR("SliderView: No view config provided");
    // }
    // update_label(volume_icon, "[Icon]", 0, 0);
    update_label(volume_icon, ICON_VOLUME_UP, 0, 0);
    update_label(volume_label, "0", 0, 20);
}

void SliderView::setupView(PB_SmartKnobConfig config) {
    setup_slider_elements(config);
    setup_circle_elements(&label_desc, &arc_dot, &arc);
    
    lv_obj_set_size(arc_dot, ARC_DOT_DIAMETER, ARC_DOT_DIAMETER);
    lv_obj_set_style_bg_color(arc_dot, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(arc_dot, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(arc_dot, ARC_DOT_OUTLINE, LV_PART_MAIN);
    lv_obj_set_style_outline_color(arc_dot, lv_color_black(), LV_PART_MAIN);

    update_label(label_desc, config.view_config.description, 0, -60);
}

void SliderView::updateView(PB_SmartKnobState state) {
    float range_radians   = (state.config.max_position - state.config.min_position) * state.config.position_width_radians;
    float left_bound_rad  = PI/2 + range_radians / 2; // 90 degrees (up) + half of range in radians
    float right_bound_rad = PI/2 - range_radians / 2; // 90 degrees (up) - half of range in radians
    float left_bound_deg = left_bound_rad * (180 / PI);
    float right_bound_deg = right_bound_rad * (180 / PI);

    float adjusted_sub_position = state.sub_position_unit * state.config.position_width_radians;
    int32_t num_positions = state.config.max_position - state.config.min_position + 1;

    update_label(volume_label, std::to_string(state.current_position).c_str(), 0, VOLUME_LABEL_POSITION(left_bound_rad));
    
    // clang-format off
    bool config_changed = (previous_state_.config.detent_strength_unit != state.config.detent_strength_unit)
        || (previous_state_.config.endstop_strength_unit != state.config.endstop_strength_unit)
        || (previous_state_.config.min_position != state.config.min_position)
        || (previous_state_.config.max_position != state.config.max_position);
    // clang-format on
    previous_state_ = state;

    if (num_positions > 0 && !state.config.infinite_scroll) {
        if (state.current_position == state.config.min_position && state.sub_position_unit < 0) {
            adjusted_sub_position = -logf(1 - state.sub_position_unit * state.config.position_width_radians / 5 / PI * 180) * 5 * PI / 180;
        } else if (state.current_position == state.config.max_position && state.sub_position_unit > 0) {
            adjusted_sub_position = logf(1 + state.sub_position_unit * state.config.position_width_radians / 5 / PI * 180) * 5 * PI / 180;
        }
    }
    
    float raw_angle_rad      = left_bound_rad - (state.current_position - state.config.min_position) * state.config.position_width_radians;
    float adjusted_angle_rad = raw_angle_rad - adjusted_sub_position;

    static float first_control_point = adjusted_angle_rad; // The control point will lerp toward the current position
    static float second_control_point = adjusted_angle_rad; // The control point will lerp toward the current position

    first_control_point = lerp_approx(first_control_point, adjusted_angle_rad, 0.25);
    second_control_point = lerp_approx(second_control_point, first_control_point, 0.25);
    
    float dot_angle_rad = second_control_point;
    // float fill_height = 0;
    // // int32_t exact_fill_height = 0;
    // if (num_positions > 1) {
    //     // exact_fill_height = 255 - state.current_position * 255 / (num_positions - 1);
    //     fill_height = 255 - second_control_point * 255 / (num_positions - 1);
    // }
    // if (config_changed) {
    //     dot_angle_rad = adjusted_angle_rad;
    //     // fill_height = exact_fill_height;
    // }
    // set_screen_gradient((int32_t)roundf(fill_height));

    int32_t raw_angle_offset_deg      = (int)(-((raw_angle_rad * (180 / PI)) - 90));
    int32_t adjusted_angle_offset_deg = (int)(-((adjusted_angle_rad * (180 / PI)) - 90));

    const bool past_minimum = (state.current_position == state.config.min_position && state.sub_position_unit < 0);
    const bool past_maximum = (state.current_position == state.config.max_position && state.sub_position_unit > 0);
    if (num_positions > 0 && !state.config.infinite_scroll && (past_minimum || past_maximum)) {
        arc_dot_set_angle(arc_dot, raw_angle_rad, ARC_PADDING + ARC_WIDTH/2, 0);
        lv_obj_clear_flag(arc, LV_OBJ_FLAG_HIDDEN);
        if (raw_angle_offset_deg < adjusted_angle_offset_deg) {
            lv_arc_set_rotation(arc, 270 + left_bound_deg);
            lv_arc_set_angles(arc, 270, 270 - (raw_angle_offset_deg - adjusted_angle_offset_deg));
        } else {
            lv_arc_set_rotation(arc, 270 + right_bound_deg);
            lv_arc_set_angles(arc, 270 - (raw_angle_offset_deg - adjusted_angle_offset_deg), 270);
        }
    } else {
        lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN);
        arc_dot_set_angle(arc_dot, dot_angle_rad, ARC_PADDING + ARC_WIDTH/2, 0);
    }
}
