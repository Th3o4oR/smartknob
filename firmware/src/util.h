#pragma once

#include <Arduino.h>

#define COUNT_OF(A) (sizeof(A) / sizeof(A[0]))

// TODO: Use template specialization to allow for different types (e.g. int, float, etc.)

/**
 * @brief Linearly map a value from one range to another.
 * 
 * Due to the linear nature of the method, a value exceeding the bounds will follow the same line mapped out between the bounds.
 * See https://www.desmos.com/calculator/obuwx9rsgu for an example
 * 
 * @param x Input value to map
 * @param in_min Input range minimum
 * @param in_max Input range maximum
 * @param out_min Output range minimum
 * @param out_max Output range maximum
 * @return float 
 * 
 * @note The run (in_max - in_min) must be non-zero.
 */
template <typename T> T remap(const T x, const T in_min, const T in_max, const T out_min, const T out_max) {
    const T run = in_max - in_min;
    assert(run != 0);
    
    const T rise = out_max - out_min;
    const T delta = x - in_min;
    return (delta * rise) / run + out_min;
}

/**
 * @brief Exponential decay function
 *
 * @param a The initial value
 * @param b The final value
 * @param decay The decay constant
 * @param dt The delta time in seconds
 * 
 * @note From: https://www.youtube.com/watch?v=LSNQuFEDOyQ
 * @note Typical decay constant: 1 - 25 (slow to fast)
 */
template <typename T> T exp_decay(const T a, const T b, const T decay, const T dt) {
    return b + (a-b)*exp(-decay*dt);
}

// TODO: The arduino library already has functions for degtorat and radtodeg

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}
