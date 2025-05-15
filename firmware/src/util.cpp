#include "util.h"

/**
 * @brief Map a value from one range to another.
 * The runs (in_max - in_min) and (out_max - out_min) must both be strictly positive.
 * 
 * @param x Input value to map
 * @param in_min Input range minimum
 * @param in_max Input range maximum
 * @param out_min Output range minimum
 * @param out_max Output range maximum
 * @return float 
 */
float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
    const float run = in_max - in_min;

    assert(in_min < in_max);
    assert(out_min < out_max);
    // if (run == 0) {
    //     return -1; // Avoid division by zero
    // }
    
    const float rise = out_max - out_min;
    const float delta = x - in_min;
    return (delta * rise) / run + out_min;
}

/**
 * @brief Clamps a value between a minimum and maximum value
 * @note From: https://stackoverflow.com/a/16659263
 */
float clamp(float min, float max, float value) {
    const float t = value < min ? min : value;
    return t > max ? max : t;
}

/**
 * @brief Linear interpolation between two values
 * @note From: https://stackoverflow.com/a/4353537
 * @note When the value is within a threshold of the target value, it will snap to the target value
 *
 * @param a The initial value
 * @param b The final value
 * @param f The fraction between the two values
 */
float lerp_approx(float a, float b, float f) {
    float new_val = a * (1.0 - f) + (b * f);
    if (fabsf(b - new_val) < 0.01) {
        return b;
    }
    return new_val;
}
