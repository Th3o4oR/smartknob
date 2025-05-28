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
void update_label(lv_obj_t *label, const char *text, lv_coord_t x_offset, lv_coord_t y_offset) {
    lv_label_set_text(label, text);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, x_offset, y_offset);
}

void SliderView::setup_slider_elements(const PB_SmartKnobConfig config) {
    float range_radians   = (config.max_position - config.min_position) * config.position_width_radians;
    float range_degrees   = range_radians * (180 / PI);
    uint16_t left_bound_deg  = roundf(270 - range_degrees / 2); // 270 degrees (up) - half of range in degrees
    uint16_t right_bound_deg = roundf(270 + range_degrees / 2); // 270 degrees (up) + half of range in degrees
    
    slider_arc = lv_arc_create(screen_);
    lv_obj_center(slider_arc);
    lv_obj_set_size(slider_arc, ARC_DIAMETER, ARC_DIAMETER);
    lv_arc_set_angles(slider_arc, left_bound_deg, right_bound_deg); // Remember that LVGL uses degrees, going clockwise from the right
    lv_arc_set_bg_angles(slider_arc, left_bound_deg, right_bound_deg);

    static lv_style_t arc_style_main, arc_style_indicator;
    lv_style_init(&arc_style_main);
    lv_style_set_arc_color(&arc_style_main, lv_palette_lighten(LV_PALETTE_GREY, 1)); // https://vuetifyjs.com/en/styles/colors/
    lv_style_set_arc_width(&arc_style_main, ARC_WIDTH);
    lv_style_set_bg_opa(&arc_style_main, LV_OPA_TRANSP);

    lv_style_init(&arc_style_indicator);
    lv_style_set_arc_width(&arc_style_indicator, ARC_WIDTH);
    lv_style_set_arc_opa(&arc_style_indicator, LV_OPA_TRANSP);

    lv_obj_add_style(slider_arc, &arc_style_main, LV_PART_MAIN);
    lv_obj_add_style(slider_arc, &arc_style_indicator, LV_PART_INDICATOR);
    lv_obj_remove_style(slider_arc, NULL, LV_PART_KNOB);

    slider_icon = lv_label_create(screen_);
    lv_obj_align(slider_icon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(slider_icon, &mdi_40, 0);
    lv_obj_set_style_text_color(slider_icon, lv_color_white(), 0);
    lv_obj_set_style_text_align(slider_icon, LV_ALIGN_CENTER, LV_PART_MAIN);
    
    slider_counter_label = lv_label_create(screen_);
    lv_obj_align(slider_counter_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(slider_counter_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(slider_counter_label, &roboto_light_24, 0);
    
    update_label(slider_icon, ICON_VOLUME_UP, 0, 0); // TODO: Use a much larger icon
}

void SliderView::setupView(PB_SmartKnobConfig config) {
    setup_slider_elements(config);
    setup_circle_elements(&label_desc, &arc_dot, &arc);

    // The arc seems to draw from the "top"/outside
    // To visually center a smaller arc on a larger arc, we add padding equal to half the width of the larger arc
    // Then, we need to subtract half the width of the smaller arc to center it
    const int32_t overshoot_indicator_width = ARC_WIDTH/2; // 1 pixel has been subtracted to account for flooring in ARC_WIDTH/2
    lv_obj_set_size(arc, ARC_DIAMETER, ARC_DIAMETER);
    lv_obj_set_style_pad_all(arc, ARC_WIDTH/2 - overshoot_indicator_width/2, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, overshoot_indicator_width, LV_PART_INDICATOR);
    
    lv_obj_set_size(arc_dot, ARC_DOT_DIAMETER, ARC_DOT_DIAMETER);
    lv_obj_set_style_bg_color(arc_dot, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_width(arc_dot, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(arc_dot, ARC_DOT_OUTLINE, LV_PART_MAIN);
    lv_obj_set_style_outline_color(arc_dot, lv_color_black(), LV_PART_MAIN);

    update_label(label_desc, "", 0, 0);

    range_radians_   = (config.max_position - config.min_position) * config.position_width_radians;
    left_bound_rad_  = PI/2 + range_radians_ / 2; // 90 degrees (up) + half of range in radians
    right_bound_rad_ = PI/2 - range_radians_ / 2; // 90 degrees (up) - half of range in radians
    left_bound_deg_  = left_bound_rad_ * (180 / PI);
    right_bound_deg_ = right_bound_rad_ * (180 / PI);
    num_positions_   = config.max_position - config.min_position + 1;
    slider_counter_position_ = SLIDER_COUNTER_POSITION(left_bound_rad_);
}

void SliderView::updateView(PB_SmartKnobState state) {
    // clang-format off
    // const bool config_changed = (previous_state_.config.detent_strength_unit != state.config.detent_strength_unit)
    // || (previous_state_.config.endstop_strength_unit != state.config.endstop_strength_unit)
    // || (previous_state_.config.min_position != state.config.min_position)
    // || (previous_state_.config.max_position != state.config.max_position);
    // clang-format on
    // previous_state_ = state;
    
    float adjusted_sub_position = state.sub_position_unit * state.config.position_width_radians;
    const bool past_minimum = (state.current_position == state.config.min_position && state.sub_position_unit < 0);
    const bool past_maximum = (state.current_position == state.config.max_position && state.sub_position_unit > 0);
    if (num_positions_ > 0 && !state.config.infinite_scroll) {
        if (past_minimum) {
            adjusted_sub_position = -logf(1 - state.sub_position_unit * state.config.position_width_radians / 5 / PI * 180) * 5 * PI / 180;
        } else if (past_maximum) {
            adjusted_sub_position = logf(1 + state.sub_position_unit * state.config.position_width_radians / 5 / PI * 180) * 5 * PI / 180;
        }
    }
    
    const float raw_angle_rad      = left_bound_rad_ - (state.current_position - state.config.min_position) * state.config.position_width_radians;
    const float adjusted_angle_rad = raw_angle_rad - adjusted_sub_position;
    // TODO: Replace with Arduino's radians() function when available
    const int32_t raw_angle_offset_deg      = (int)(-((raw_angle_rad * (180 / PI)) - 90));
    const int32_t adjusted_angle_offset_deg = (int)(-((adjusted_angle_rad * (180 / PI)) - 90));

    // Calculate delta time since last frame. TODO: Pass this in from the display task instead of calculating it here
    static TickType_t tick = xTaskGetTickCount();
    TickType_t current_tick = xTaskGetTickCount();
    TickType_t delta_tick = current_tick - tick;
    tick = current_tick; // Update the tick for the next frame
    float dt = static_cast<float>(delta_tick) / configTICK_RATE_HZ; // Convert to seconds

    constexpr float decay_slow = 20.0f; // Decay factor for the exponential decay function
    constexpr float decay_fast = 35.0f; // Faster decay for the dot when past bounds
    static float decay = decay_slow;
    static float first_control_point = adjusted_angle_rad;
    static float second_control_point = adjusted_angle_rad;
    first_control_point = exp_decay<float>(first_control_point, adjusted_angle_rad, decay, dt);
    second_control_point = exp_decay<float>(second_control_point, first_control_point, decay, dt);
    float dot_angle_rad = std::clamp(second_control_point, right_bound_rad_, left_bound_rad_);

    if (num_positions_ > 0 && !state.config.infinite_scroll && (past_minimum || past_maximum)) {
        decay = decay_fast;

        const bool dot_at_right_bound = fabs(dot_angle_rad - right_bound_rad_) < 0.01f;
        const bool dot_at_left_bound  = fabs(dot_angle_rad - left_bound_rad_) < 0.01f;
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
        decay = decay_slow;
    }

    arc_dot_set_angle(arc_dot, dot_angle_rad, ARC_DOT_PADDING, 0);

    lv_label_set_text_fmt(slider_counter_label, "%d", state.current_position);
    lv_obj_set_style_text_align(slider_counter_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(slider_counter_label, LV_ALIGN_CENTER, 0, slider_counter_position_);
}
