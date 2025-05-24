#include "dial_view.h"
#include "styles.h"
#include "view_util.h"

void DialView::setup_dial_elements(PB_SmartKnobConfig config) {
    line_left_bound = lv_line_create(screen_);
    lv_obj_add_style(line_left_bound, &style_line, 0);
    lv_obj_set_pos(line_left_bound, 0, 0);
    
    line_right_bound = lv_line_create(screen_);
    lv_obj_add_style(line_right_bound, &style_line, 0);
    lv_obj_set_pos(line_right_bound, 0, 0);

    label_cur_pos = lv_label_create(screen_);
    lv_obj_set_style_text_align(label_cur_pos, LV_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(label_cur_pos, LV_ALIGN_CENTER, 0, -25);
    lv_label_set_text(label_cur_pos, "0");
    lv_obj_set_style_text_color(label_cur_pos, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_cur_pos, &roboto_light_60, 0);

    lv_obj_align(label_desc, LV_ALIGN_CENTER, 0, DIAL_DESCRIPTION_Y_OFFSET);
}

void DialView::setupView(PB_SmartKnobConfig config) {
    setup_circle_elements(&label_desc, &arc_dot, &arc);
    setup_dial_elements(config);

    lv_label_set_text(label_desc, config.view_config.description);
    lv_obj_set_style_text_align(label_desc, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(label_desc, LV_ALIGN_CENTER, 0, DIAL_DESCRIPTION_Y_OFFSET); // Realigns label and its content to the center - this is required every time after setting text value

    // Assume that the bounds don't change, so we can calculate them once
    range_radians_   = (config.max_position - config.min_position) * config.position_width_radians;
    left_bound_rad_  = PI/2 + range_radians_ / 2; // 90 degrees (up) + half of range in radians
    right_bound_rad_ = PI/2 - range_radians_ / 2; // 90 degrees (up) - half of range in radians
    left_bound_deg_ = left_bound_rad_ * (180 / PI);
    right_bound_deg_ = right_bound_rad_ * (180 / PI);
    num_positions_ = config.max_position - config.min_position + 1;

    if (num_positions_ > 0 && config.infinite_scroll == 0) {
        points_left_bound[0] = radial_coordinates(left_bound_rad_, RADIUS,      RADIUS); // Unable to use alignment with lines, so we use the offset to center
        points_left_bound[1] = radial_coordinates(left_bound_rad_, RADIUS - 10, RADIUS); // Unable to use alignment with lines, so we use the offset to center
        lv_line_set_points(line_left_bound, points_left_bound, 2);
        points_right_bound[0] = radial_coordinates(right_bound_rad_, RADIUS,      RADIUS); // Unable to use alignment with lines, so we use the offset to center
        points_right_bound[1] = radial_coordinates(right_bound_rad_, RADIUS - 10, RADIUS); // Unable to use alignment with lines, so we use the offset to center
        lv_line_set_points(line_right_bound, points_right_bound, 2);
    } else {
        points_left_bound[0] = {0,0};
        points_left_bound[1] = {0,0};
        lv_line_set_points(line_left_bound, points_left_bound, 2);
        points_right_bound[0] = {0,0};
        points_right_bound[1] = {0,0};
        lv_line_set_points(line_right_bound, points_right_bound, 2);
    }
}

void DialView::updateView(PB_SmartKnobState state) {
    // // clang-format off
    // bool config_changed = (previous_state_.config.detent_strength_unit != state.config.detent_strength_unit)
    //     || (previous_state_.config.endstop_strength_unit != state.config.endstop_strength_unit)
    //     || (previous_state_.config.min_position != state.config.min_position)
    //     || (previous_state_.config.max_position != state.config.max_position);
    // // clang-format on
    // previous_state_ = state;
    
    const bool past_minimum = (state.current_position == state.config.min_position && state.sub_position_unit < 0);
    const bool past_maximum = (state.current_position == state.config.max_position && state.sub_position_unit > 0);
    float adjusted_sub_position = state.sub_position_unit * state.config.position_width_radians;
    if (num_positions_ > 0 && state.config.infinite_scroll == 0) {
        if (past_minimum) {
            adjusted_sub_position = -logf(1 - state.sub_position_unit * state.config.position_width_radians / 5 / PI * 180) * 5 * PI / 180;
        } else if (past_maximum) {
            adjusted_sub_position = logf(1 + state.sub_position_unit * state.config.position_width_radians / 5 / PI * 180) * 5 * PI / 180;
        }
    }

    float raw_angle_rad      = left_bound_rad_ - (state.current_position - state.config.min_position) * state.config.position_width_radians;
    float adjusted_angle_rad = raw_angle_rad - adjusted_sub_position;
    int32_t raw_angle_offset_deg      = (int)(-((raw_angle_rad * (180 / PI)) - 90));
    int32_t adjusted_angle_offset_deg = (int)(-((adjusted_angle_rad * (180 / PI)) - 90));

    static TickType_t tick = xTaskGetTickCount(); // Reset the tick count to avoid using the same value for lerp_approx in the next frame
    TickType_t current_tick = xTaskGetTickCount();
    TickType_t delta_tick = current_tick - tick;
    tick = current_tick; // Update the tick for the next frame
    float dt = (float)delta_tick / configTICK_RATE_HZ; // Convert to seconds

    constexpr float decay_slow = 20.0f; // Decay factor for the exponential decay function
    constexpr float decay_fast = 35.0f; // Faster decay for the dot when past bounds
    static float decay = decay_slow;
    static float first_control_point = adjusted_angle_rad; // The control point will lerp toward the current position
    static float second_control_point = adjusted_angle_rad; // The control point will lerp toward the current position
    first_control_point = exp_decay(first_control_point, adjusted_angle_rad, decay, dt);
    second_control_point = exp_decay(second_control_point, first_control_point, decay, dt);
    float dot_angle_rad = clamp(second_control_point, right_bound_rad_, left_bound_rad_);

    if (num_positions_ > 0 && !state.config.infinite_scroll && (past_minimum || past_maximum)) {
        decay = decay_fast;

        const bool dot_at_right_bound = fabs(dot_angle_rad - right_bound_rad_) < 0.01f; // Allow a small margin of error
        const bool dot_at_left_bound  = fabs(dot_angle_rad - left_bound_rad_) < 0.01f; // Allow a small margin of error
        if (dot_at_right_bound && raw_angle_offset_deg < adjusted_angle_offset_deg) {
            lv_obj_clear_flag(arc, LV_OBJ_FLAG_HIDDEN);
            lv_arc_set_rotation(arc, 270 + left_bound_deg_);
            lv_arc_set_angles(arc, 270, 270 - (raw_angle_offset_deg - adjusted_angle_offset_deg));
        } else if (dot_at_left_bound && raw_angle_offset_deg > adjusted_angle_offset_deg) {
            lv_obj_clear_flag(arc, LV_OBJ_FLAG_HIDDEN);
            lv_arc_set_rotation(arc, 270 + right_bound_deg_);
            lv_arc_set_angles(arc, 270 - (raw_angle_offset_deg - adjusted_angle_offset_deg), 270);
        } else {
            lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN);
        decay = decay_slow; // Reset decay to the default value
    }

    // const float dot_angle_rad = clamp(second_control_point, right_bound_rad_, left_bound_rad_);
    float fill_height = 0;
    // int32_t exact_fill_height = 0;
    if (num_positions_ > 1) {
        // exact_fill_height = 255 - state.current_position * 255 / (num_positions - 1);
        fill_height = clamp(
            mapf(second_control_point, left_bound_rad_, right_bound_rad_, 255, 0),
            0.0f,
            255.0f
        );
    }
    
    arc_dot_set_angle(arc_dot, dot_angle_rad, arc_dot_dial_padding, arc_dot_size);
    // This causes significant slow down
    // However, exponential decay accounts for the delta time
    // Thus, the movement will be smooth regardless of the frame rate, and matches the speed of all other views
    set_screen_gradient((int32_t)roundf(fill_height));
    
    lv_label_set_text_fmt(label_cur_pos, "%d", state.current_position);
    lv_obj_set_style_text_align(label_cur_pos, LV_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(label_cur_pos, LV_ALIGN_CENTER, 0, -25);
}
