#include <Arduino.h>
#include "view.h"
#include "view_util.h"
#include "styles.h"

/**
 * @brief Set the angle of the arc dot object
 * 
 * @param arc_dot The arc dot object
 * @param angle The angle to set the dot to (in radians)
 * @param padding Any padding to apply to the dot
 * @param dot_size The size of the dot
 */
void arc_dot_set_angle(lv_obj_t *arc_dot, float angle, int padding, int dot_size) {
    // TODO: Dot size should not be included in the padding.
    // The dot is center aligned, so the x- and y-offsets are calculated from the center of the dot

    float reduced_radius   = RADIUS - padding - dot_size / 2;
    // float offset_to_center = RADIUS - dot_size / 2; // Unneeded when aligning to center
    lv_point_t arc_coords = radial_coordinates(angle, reduced_radius);
    lv_obj_align(arc_dot, LV_ALIGN_CENTER, arc_coords.x, arc_coords.y);
    // lv_obj_set_style_transform_angle(arc_dot, angle * 180 / PI, LV_PART_MAIN); // In case you want to rotate the dot itself
}

void set_screen_gradient(const int value, const lv_color_t color) {
    lv_obj_t * screen = lv_scr_act();
    lv_obj_set_style_bg_grad_stop(screen, value, LV_PART_MAIN);
    lv_obj_set_style_bg_main_stop(screen, value, LV_PART_MAIN); // Sets the same value for gradient and main - gives sharp line on gradient

    lv_obj_set_style_bg_grad_color(screen, color, LV_PART_MAIN);
}

void setup_circle_elements(lv_obj_t **label_desc, lv_obj_t **arc_dot, lv_obj_t **arc) {
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    // lv_obj_set_style_bg_grad_color(screen, FILL_COLOR, LV_PART_MAIN);   
    lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_VER, LV_PART_MAIN);
    set_screen_gradient(255, FILL_COLOR);

    *label_desc = lv_label_create(screen);
    lv_label_set_text(*label_desc, "");
    lv_obj_set_style_text_align(*label_desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(*label_desc, LV_ALIGN_CENTER, 0, MENU_DESCRIPTION_Y_OFFSET);
    lv_obj_set_style_text_color(*label_desc, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(*label_desc, &roboto_light_24, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(*label_desc, 1, LV_PART_MAIN);

    *arc = lv_arc_create(screen);
    lv_obj_set_size(*arc, TFT_WIDTH, TFT_HEIGHT);
    lv_obj_align(*arc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(*arc, 8, LV_PART_MAIN);
    lv_arc_set_bg_angles(*arc, 0, 0);              // Hides arc background by setting background arc angles to 0
    lv_obj_remove_style(*arc, NULL, LV_PART_KNOB); // Hides arc knob - for arc on the boundaries we only want the indicator part of the lvgl arc widget
    lv_obj_set_style_arc_color(*arc, DOT_COLOR, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(*arc, 5, LV_PART_INDICATOR);
    lv_arc_set_mode(*arc, LV_ARC_MODE_REVERSE);

    *arc_dot = lv_obj_create(screen);
    lv_obj_set_scrollbar_mode(*arc_dot, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(*arc_dot, arc_dot_size, arc_dot_size);
    arc_dot_set_angle(*arc_dot, 0, arc_dot_menu_padding, arc_dot_size);
    lv_obj_set_style_bg_color(*arc_dot, DOT_COLOR, 0);
    lv_obj_set_style_radius(*arc_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(*arc_dot, lv_color_white(), 0);
    lv_obj_set_style_border_width(*arc_dot, 2, 0);
}

/**
 * @brief Set the up circle button object
 *
 * @param button Which button to set up
 * @param index The index of the button in the menu
 * @param config The configuration for the menu
 * @param button_event_cb The event callback for the button
 */
void setup_circle_button(
  lv_obj_t               **button,
  const int                index,
  const PB_SmartKnobConfig &knob_config,
  lv_event_cb_t            button_event_cb
) {
    lv_obj_t *screen = lv_scr_act();
    *button          = lv_btn_create(screen);
    lv_obj_t *label  = lv_label_create(*button);

    lv_label_set_text(label, knob_config.view_config.menu_entries[index].icon);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label, &mdi_40, LV_PART_MAIN);

    lv_obj_set_size(*button, button_size, button_size);
    lv_obj_add_style(*button, &circle_btn_style, 0);
    lv_obj_add_style(*button, &circle_btn_highlighted_style, LV_STATE_FOCUSED);

    lv_obj_set_user_data(*button, (void *)index);
    lv_obj_add_event_cb(*button, button_event_cb, LV_EVENT_PRESSED, NULL);

    float      reduced_radius = RADIUS - button_padding - button_size / 2;
    float      start_angle   = calculate_start_angle(knob_config);
    float      button_angle  = start_angle - index * knob_config.position_width_radians;
    lv_point_t button_coords = radial_coordinates(button_angle, reduced_radius);
    lv_obj_align(*button, LV_ALIGN_CENTER, button_coords.x, button_coords.y);
}

/**
 * @brief Calculate the start angle for the buttons in the circle menu
 * @note We have assumed that the conversion from angle to position is based on:
 * @note An angle of 0 degrees is at the right of the screen
 * @note An angle of 90 degrees is at the top of the screen, etc.
 *
 * @param num_buttons The number of buttons in the menu
 * @param angle_step The angle step between each button
 */
float calculate_start_angle(PB_SmartKnobConfig knob_config) {
    if (knob_config.has_view_config && knob_config.view_config.view_type != VIEW_CIRCLE_MENU) {
        return PI / 2; // 90 degrees (up)
    }
    int   num_buttons = knob_config.max_position + 1;
    float angle_step  = knob_config.position_width_radians;
    return PI / 2 + ((num_buttons - 1) * angle_step) / 2.0f; // Centered around the top of the screen
}

/**
 * @brief Calculate the x and y coordinates of a point on a circle given an angle, radius and offset
 *
 * @note You should also use center alignment for the object you are drawing, so that the coordinates are relative to the center of the circle.
 * @note If you can't use center alignment, you can use the offset parameter to account move the coordinates to the center of the circle.
 *
 * @param angle The angle in radians
 * @param radius The distance from the center of the circle to the point
 * @param offset How much to offset the coordinates to account for the size of whatever is being drawn (counted from the top left corner of whatever is being drawn)
 * @return lv_point_t
 */
lv_point_t radial_coordinates(float angle, float radius, float offset) {
    lv_point_t point;
    point.x = (lv_coord_t)(offset + radius * cosf(angle));
    point.y = (lv_coord_t)(offset - radius * sinf(angle));
    return point;
}
