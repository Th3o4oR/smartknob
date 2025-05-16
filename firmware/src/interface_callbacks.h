#pragma once

#include <functional>
#include "proto_gen/smartknob.pb.h"

// Forward declaration of PageType (must match the definition in page.h)
enum class PageType : int;

/**
 * @brief Callback to request a page change
 * 
 * @param page The requested page
 */
typedef std::function<void(PageType)> PageChangeCallback;

/**
 * @brief Callback to request a config change
 * 
 * @param config The new config
 */
typedef std::function<void(PB_SmartKnobConfig&)> ConfigCallback;

/**
 * @brief Callback to start motor calibration
 */
typedef std::function<void(void)> MotorCalibrationCallback;
