#pragma once

#include <Arduino.h>

template <typename T> T CLAMP(const T& value, const T& low, const T& high) 
{
  return value < low ? low : (value > high ? high : value); 
}

#define COUNT_OF(A) (sizeof(A) / sizeof(A[0]))

float mapf(float x, float in_min, float in_max, float out_min, float out_max);
float clamp(float min, float max, float value);
float lerp_approx(float a, float b, float f);

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}
