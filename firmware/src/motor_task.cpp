#include <SimpleFOC.h>

#include "motor_task.h"
#if SENSOR_MT6701
#include "mt6701_sensor.h"
#elif SENSOR_TLV
#include "tlv_sensor.h"
#elif SENSOR_MAQ430
#include "maq430_sensor.h"
#endif

#include "motors/motor_config.h"
#include "util.h"

// Empirically determined values for the knob with red wires
/*
    (absolute) maximum shaft velocity while idle (not exactly at detent center): 0.33
    (absolute) maximum shaft velocity while at detent center: 0.29
    Angle difference where motor is unable to move: 0.12 (set to 0.15 for safety)
        0.12 rad = 6.87 degrees => Let's accept 8 degrees (maybe even 10?)
*/

static const float DEAD_ZONE_DETENT_PERCENT = 0.2;
static const float DEAD_ZONE_RAD = 1 * _PI / 180;

static const float IDLE_VELOCITY_EWMA_ALPHA = 0.001;
static const float IDLE_VELOCITY_RAD_PER_SEC = 0.05;
static const uint32_t IDLE_CORRECTION_DELAY_MILLIS = 500;
static const float IDLE_CORRECTION_MAX_ANGLE_RAD = 8 * PI / 180;
static const float IDLE_CORRECTION_RATE_ALPHA = 0.0005;


MotorTask::MotorTask(const uint8_t task_core, const uint32_t stack_depth, Configuration& configuration)
    : Task("Motor", stack_depth, 1, task_core), configuration_(configuration) {
    queue_ = xQueueCreate(5, sizeof(Command));
    assert(queue_ != NULL);
}

MotorTask::~MotorTask() {}


#if SENSOR_TLV
    TlvSensor encoder = TlvSensor();
#elif SENSOR_MT6701
    MT6701Sensor encoder = MT6701Sensor();
#elif SENSOR_MAQ430
    MagneticSensorSPI encoder = MagneticSensorSPI(MAQ430_SPI, PIN_MAQ_SS);
#endif

void MotorTask::run() {

    driver.voltage_power_supply = 5;
    driver.init();

    #if SENSOR_TLV
    encoder.init(&Wire, false);
    #elif SENSOR_MT6701
    encoder.init();
    #elif SENSOR_MAQ430
    SPIClass* spi = new SPIClass(HSPI);
    spi->begin(PIN_MAQ_SCK, PIN_MAQ_MISO, PIN_MAQ_MOSI, PIN_MAQ_SS);
    encoder.init(spi);
    #endif

    motor.linkDriver(&driver);

    motor.controller = MotionControlType::torque;
    motor.voltage_limit = FOC_VOLTAGE_LIMIT;
    motor.velocity_limit = 10000;
    motor.linkSensor(&encoder);

    // Not actually using the velocity loop built into SimpleFOC; but I'm using those PID variables
    // to run PID for torque (and SimpleFOC studio supports updating them easily over serial for tuning)
    motor.PID_velocity.P = FOC_PID_P;
    motor.PID_velocity.I = FOC_PID_I;
    motor.PID_velocity.D = FOC_PID_D;
    motor.PID_velocity.output_ramp = FOC_PID_OUTPUT_RAMP;
    motor.PID_velocity.limit = FOC_PID_LIMIT;

    #ifdef FOC_LPF
    motor.LPF_angle.Tf = FOC_LPF;
    #endif

    motor.init();

    encoder.update();
    delay(10);

    PB_PersistentConfiguration c = configuration_.get();
    motor.pole_pairs = c.motor.calibrated ? c.motor.pole_pairs : 7;
    motor.initFOC(c.motor.zero_electrical_offset, c.motor.direction_cw ? Direction::CW : Direction::CCW);

    motor.monitor_downsample = 0; // disable monitor at first - optional

    // disableCore0WDT();

    float current_detent_center = motor.shaft_angle;
    PB_SmartKnobConfig config = {
        .initial_position = 0,
        .sub_position_unit = 0,
        .position_nonce = 0,
        .min_position = 0,
        .max_position = 1,
        .position_width_radians = 60 * _PI / 180,
        .detent_strength_unit = 0,
    };
    int32_t current_position = 0;
    float latest_sub_position_unit = 0;

    float idle_check_velocity_ewma = 0;
    uint32_t last_idle_start = 0;
    uint32_t last_publish = 0;

    while (1) {
        motor.loopFOC();

        // Check queue for pending requests from other tasks
        Command command;
        if (xQueueReceive(queue_, &command, 0) == pdTRUE) {
            switch (command.command_type) {
                case CommandType::CALIBRATE:
                    calibrate();
                    break;
                case CommandType::CONFIG: {
                    // Check new config for validity
                    PB_SmartKnobConfig& new_config = command.data.config;
                    if (new_config.detent_strength_unit < 0) {
                        LOG_WARN("Ignoring invalid config: detent_strength_unit cannot be negative");
                        break;
                    }
                    if (new_config.endstop_strength_unit < 0) {
                        LOG_WARN("Ignoring invalid config: endstop_strength_unit cannot be negative");
                        break;
                    }
                    if (new_config.snap_point < 0.5) {
                        LOG_WARN("Ignoring invalid config: snap_point must be >= 0.5 for stability");
                        break;
                    }
                    if (new_config.detent_positions_count > COUNT_OF(new_config.detent_positions)) {
                        LOG_WARN("Ignoring invalid config: detent_positions_count is too large");
                        break;
                    }
                    if (new_config.snap_point_bias < 0) {
                        LOG_WARN("Ignoring invalid config: snap_point_bias cannot be negative or there is risk of instability");
                        break;
                    }

                    // Change haptic input mode
                    bool position_updated = false;
                    if (new_config.initial_position != config.initial_position
                            || new_config.sub_position_unit != config.sub_position_unit
                            || new_config.position_nonce != config.position_nonce) {
                        LOG_INFO("applying position change");
                        current_position = new_config.initial_position;
                        position_updated = true;
                    }

                    if (new_config.min_position <= new_config.max_position) {
                        // Only check bounds if min/max indicate bounds are active (min >= max)
                        if (current_position < new_config.min_position) {
                            current_position = new_config.min_position;
                            LOG_INFO("adjusting position to min");
                        } else if (current_position > new_config.max_position) {
                            current_position = new_config.max_position;
                            LOG_INFO("adjusting position to max");
                        }
                    }

                    if (position_updated || new_config.position_width_radians != config.position_width_radians) {
                        LOG_INFO("adjusting detent center");
                        float new_sub_position = position_updated ? new_config.sub_position_unit : latest_sub_position_unit;
                        #if SK_INVERT_ROTATION
                            float shaft_angle = -motor.shaft_angle;
                        #else
                            float shaft_angle = motor.shaft_angle;
                        #endif
                        current_detent_center = shaft_angle + new_sub_position * new_config.position_width_radians;
                    }
                    config = new_config;
                    LOG_INFO("Got new config");

                    // Update derivative factor of torque controller based on detent width.
                    // If the D factor is large on coarse detents, the motor ends up making noise because the P&D factors amplify the noise from the sensor.
                    // This is a piecewise linear function so that fine detents (small width) get a higher D factor and coarse detents get a small D factor.
                    // Fine detents need a nonzero D factor to artificially create "clicks" each time a new value is reached (the P factor is small
                    // for fine detents due to the smaller angular errors, and the existing P factor doesn't work well for very small angle changes (easy to
                    // get runaway due to sensor noise & lag)).
                    // TODO: consider eliminating this D factor entirely and just "play" a hardcoded haptic "click" (e.g. a quick burst of torque in each
                    // direction) whenever the position changes when the detent width is too small for the P factor to work well.
                    const float derivative_lower_strength = config.detent_strength_unit * 0.08;
                    const float derivative_upper_strength = config.detent_strength_unit * 0.02;
                    const float derivative_position_width_lower = radians(3);
                    const float derivative_position_width_upper = radians(8);
                    const float raw = derivative_lower_strength + (derivative_upper_strength - derivative_lower_strength)/(derivative_position_width_upper - derivative_position_width_lower)*(config.position_width_radians - derivative_position_width_lower);
                    // When there are intermittent detents (set via detent_positions), disable derivative factor as this adds extra "clicks" when nearing
                    // a detent.
                    motor.PID_velocity.D = config.detent_positions_count > 0 ? 0 : CLAMP(
                        raw,
                        min(derivative_lower_strength, derivative_upper_strength),
                        max(derivative_lower_strength, derivative_upper_strength)
                    );
                    break;
                }
                case CommandType::HAPTIC: {
                    // Play a hardcoded haptic "click"
                    float strength = command.data.haptic.press ? 5 : 1.5;
                    motor.move(strength);
                    for (uint8_t i = 0; i < 3; i++) {
                        motor.loopFOC();
                        delay(1);
                    }
                    motor.move(-strength);
                    for (uint8_t i = 0; i < 3; i++) {
                        motor.loopFOC();
                        delay(1);
                    }
                    motor.move(0);
                    motor.loopFOC();
                    break;
                }
            }
        }

        // If we are not moving and we're close to the center (but not exactly there), slowly adjust the centerpoint to match the current position
        idle_check_velocity_ewma = motor.shaft_velocity * IDLE_VELOCITY_EWMA_ALPHA + idle_check_velocity_ewma * (1 - IDLE_VELOCITY_EWMA_ALPHA);
        if (fabsf(idle_check_velocity_ewma) > IDLE_VELOCITY_RAD_PER_SEC) {
            last_idle_start = 0;
        } else {
            if (last_idle_start == 0) {
                last_idle_start = millis();
            }
        }
        if (last_idle_start > 0 && millis() - last_idle_start > IDLE_CORRECTION_DELAY_MILLIS && fabsf(motor.shaft_angle - current_detent_center) < IDLE_CORRECTION_MAX_ANGLE_RAD) {
            current_detent_center = motor.shaft_angle * IDLE_CORRECTION_RATE_ALPHA + current_detent_center * (1 - IDLE_CORRECTION_RATE_ALPHA);
        }

        // // Log current motor state
        // char motor_buf[256];
        // static float global_maximum_absolute_velocity = 0;
        // static float local_maximum_absolute_velocity = 0;
        // static uint32_t shaft_velocity_count = 0;
        // shaft_velocity_count++;
        // if (fabsf(motor.shaft_velocity) > global_maximum_absolute_velocity) {
        //     global_maximum_absolute_velocity = fabs(motor.shaft_velocity);
        // }
        // if (fabs(motor.shaft_velocity) > local_maximum_absolute_velocity) {
        //     local_maximum_absolute_velocity = fabs(motor.shaft_velocity);
        // }
        // static uint32_t last_motor_shaft_angle_log = 0;
        // if (millis() - last_motor_shaft_angle_log > 250) {
        //     snprintf(
        //         motor_buf,
        //         sizeof(motor_buf),
        //         "angle:% 5.2f"
        //         ", max_local_velocity:% 5.2f"
        //         ", ewma_velocity:% 5.2f"
        //         ", current_detent_center:% 5.2f"
        //         ,motor.shaft_angle
        //         ,local_maximum_absolute_velocity
        //         ,idle_check_velocity_ewma
        //         ,current_detent_center
        //     );
        //     log(motor_buf);
        //     local_maximum_absolute_velocity = fabs(motor.shaft_velocity);
        //     shaft_velocity_count = 0;
        //     last_motor_shaft_angle_log = millis();
        // }

        // Check where we are relative to the current nearest detent; update our position if we've moved far enough to snap to another detent
        float angle_to_detent_center = motor.shaft_angle - current_detent_center;  // Positive means the physical sensor is to the "right" of the detent center
        #if SK_INVERT_ROTATION
            angle_to_detent_center = -motor.shaft_angle - current_detent_center;
        #endif

        float snap_point_radians = config.position_width_radians * config.snap_point;
        float bias_radians = config.position_width_radians * config.snap_point_bias;
        float snap_point_radians_decrease = -snap_point_radians + (current_position <= 0 ? -bias_radians : bias_radians);
        float snap_point_radians_increase = snap_point_radians + (current_position >= 0 ? bias_radians : -bias_radians); 

        int32_t num_positions = config.max_position - config.min_position + 1;
        if (angle_to_detent_center > snap_point_radians_increase && (num_positions <= 0 || current_position < config.max_position || config.infinite_scroll)) {
            current_detent_center += config.position_width_radians;
            angle_to_detent_center -= config.position_width_radians;
            current_position++;
        } else if (angle_to_detent_center < snap_point_radians_decrease && (num_positions <= 0 || current_position > config.min_position || config.infinite_scroll)) {
            current_detent_center -= config.position_width_radians;
            angle_to_detent_center += config.position_width_radians;
            current_position--;
        }

        if (config.infinite_scroll) {
            if (current_position > config.max_position) {
                current_position = config.min_position;
            } else if (current_position < config.min_position) {
                current_position = config.max_position;
            }
        }

        latest_sub_position_unit = angle_to_detent_center / config.position_width_radians;

        float dead_zone_adjustment = CLAMP(
            angle_to_detent_center,
            fmaxf(-config.position_width_radians*DEAD_ZONE_DETENT_PERCENT, -DEAD_ZONE_RAD),
            fminf(config.position_width_radians*DEAD_ZONE_DETENT_PERCENT, DEAD_ZONE_RAD));

        bool out_of_bounds = num_positions > 0 && !config.infinite_scroll && ((angle_to_detent_center > 0 && current_position == config.max_position) || (angle_to_detent_center < 0 && current_position == config.min_position));
        motor.PID_velocity.limit = 10; //out_of_bounds ? 10 : 3;
        motor.PID_velocity.P = out_of_bounds ? config.endstop_strength_unit * 4 : config.detent_strength_unit * 4;


        // Apply motor torque based on our angle to the nearest detent (detent strength, etc is handled by the PID_velocity parameters)
        if (fabsf(motor.shaft_velocity) > 60) {
            // Don't apply torque if velocity is too high (helps avoid positive feedback loop/runaway)
            motor.move(0);
        } else {
            float input = -angle_to_detent_center + dead_zone_adjustment;
            if (!out_of_bounds && config.detent_positions_count > 0) {
                bool in_detent = false;
                for (uint8_t i = 0; i < config.detent_positions_count; i++) {
                    if (config.detent_positions[i] == current_position) {
                        in_detent = true;
                        break;
                    }
                }
                if (!in_detent) {
                    input = 0;
                }
            }
            float torque = motor.PID_velocity(input);
            #if SK_INVERT_ROTATION
                torque = -torque;
            #endif
            motor.move(torque);
        }

        // Publish current status to other registered tasks periodically
        if (millis() - last_publish > 5) {
            publish({
                .current_position = current_position,
                .sub_position_unit = latest_sub_position_unit,
                .has_config = true,
                .config = config,
            });
            last_publish = millis();
        }

        delay(1);
    }
}

void MotorTask::setConfig(const PB_SmartKnobConfig& config) {
    Command command = {
        .command_type = CommandType::CONFIG,
        .data = {
            .config = config,
        }
    };
    xQueueSend(queue_, &command, portMAX_DELAY);
}


void MotorTask::playHaptic(bool press) {
    Command command = {
        .command_type = CommandType::HAPTIC,
        .data = {
            .haptic = {
                .press = press,
            },
        }
    };
    xQueueSend(queue_, &command, portMAX_DELAY);
}

void MotorTask::runCalibration() {
    Command command = {
        .command_type = CommandType::CALIBRATE,
        .data = {
            .unused = 0,
        }
    };
    xQueueSend(queue_, &command, portMAX_DELAY);
}


void MotorTask::addListener(QueueHandle_t queue) {
    listeners_.push_back(queue);
}

void MotorTask::publish(const PB_SmartKnobState& state) {
    for (auto listener : listeners_) {
        xQueueOverwrite(listener, &state);
    }
}

void MotorTask::calibrate() {
    // SimpleFOC is supposed to be able to determine this automatically (if you omit params to initFOC), but
    // it seems to have a bug (or I've misconfigured it) that gets both the offset and direction very wrong!
    // So this value is based on experimentation.
    // TODO: dig into SimpleFOC calibration and find/fix the issue

    LOG_INFO("\n\n\nStarting calibration, please DO NOT TOUCH MOTOR until complete!");
    delay(1000);

    motor.controller = MotionControlType::angle_openloop;
    motor.pole_pairs = 1;
    motor.initFOC(0, Direction::CW);

    float a = 0;

    motor.voltage_limit = FOC_VOLTAGE_LIMIT;
    motor.move(a);

    // #### Determine direction motor rotates relative to angle sensor
    for (uint8_t i = 0; i < 200; i++) {
        encoder.update();
        motor.move(a);
        delay(1);
    }
    float start_sensor = encoder.getAngle();

    for (; a < 3 * _2PI; a += 0.01) {
        encoder.update();
        motor.move(a);
        delay(1);
    }

    for (uint8_t i = 0; i < 200; i++) {
        encoder.update();
        delay(1);
    }
    float end_sensor = encoder.getAngle();


    motor.voltage_limit = 0;
    motor.move(a);

    LOG_INFO("");

    float movement_angle = fabsf(end_sensor - start_sensor);
    if (movement_angle < radians(30) || movement_angle > radians(180)) {
        LOG_ERROR("Unexpected sensor change: start=%.2f end=%.2f", start_sensor, end_sensor);
        return;
    }

    LOG_INFO("Sensor measures positive for positive motor rotation:");
    if (end_sensor > start_sensor) {
        LOG_INFO("YES, Direction=CW");
        motor.initFOC(0, Direction::CW);
    } else {
        LOG_WARN("NO, Direction=CCW");
        motor.initFOC(0, Direction::CCW);
    }
    LOG_INFO("  (start was %.1f, end was %.1f)", start_sensor, end_sensor);

    // #### Determine pole-pairs
    // Rotate 20 electrical revolutions and measure mechanical angle traveled, to calculate pole-pairs
    uint8_t electrical_revolutions = 20;
    LOG_INFO("Going to measure %d electrical revolutions...", electrical_revolutions);
    motor.voltage_limit = FOC_VOLTAGE_LIMIT;
    motor.move(a);
    LOG_INFO("Going to electrical zero...");
    float destination = a + _2PI;
    for (; a < destination; a += 0.03) {
        encoder.update();
        motor.move(a);
        delay(1);
    }
    LOG_INFO("pause..."); // Let momentum settle...
    for (uint16_t i = 0; i < 1000; i++) {
        encoder.update();
        delay(1);
    }
    LOG_INFO("Measuring...");

    start_sensor = motor.sensor_direction * encoder.getAngle();
    destination = a + electrical_revolutions * _2PI;
    for (; a < destination; a += 0.03) {
        encoder.update();
        motor.move(a);
        delay(1);
    }
    for (uint16_t i = 0; i < 1000; i++) {
        encoder.update();
        motor.move(a);
        delay(1);
    }
    end_sensor = motor.sensor_direction * encoder.getAngle();
    motor.voltage_limit = 0;
    motor.move(a);

    if (fabsf(motor.shaft_angle - motor.target) > 1 * PI / 180) {
        LOG_ERROR("Motor did not reach target!");
        while(1) {}
    }

    float electrical_per_mechanical = electrical_revolutions * _2PI / (end_sensor - start_sensor);
    LOG_INFO("Electrical angle / mechanical angle (i.e. pole pairs) = %.2f", electrical_per_mechanical);

    if (electrical_per_mechanical < 3 || electrical_per_mechanical > 12) {
        LOG_ERROR("Unexpected calculated pole pairs: %.2f", electrical_per_mechanical);
        return;
    }

    int measured_pole_pairs = (int)round(electrical_per_mechanical);
    LOG_INFO("Pole pairs set to %d", measured_pole_pairs);

    delay(1000);


    // #### Determine mechanical offset to electrical zero
    // Measure mechanical angle at every electrical zero for several revolutions
    motor.voltage_limit = FOC_VOLTAGE_LIMIT;
    motor.move(a);
    float offset_x = 0;
    float offset_y = 0;
    float destination1 = (floor(a / _2PI) + measured_pole_pairs / 2.) * _2PI;
    float destination2 = (floor(a / _2PI)) * _2PI;
    for (; a < destination1; a += 0.4) {
        motor.move(a);
        delay(100);
        for (uint8_t i = 0; i < 100; i++) {
            encoder.update();
            delay(1);
        }
        float real_electrical_angle = _normalizeAngle(a);
        float measured_electrical_angle = _normalizeAngle( (float)(motor.sensor_direction * measured_pole_pairs) * encoder.getMechanicalAngle()  - 0);

        float offset_angle = measured_electrical_angle - real_electrical_angle;
        offset_x += cosf(offset_angle);
        offset_y += sinf(offset_angle);

        LOG_INFO("%.2f, %.2f, %.2f", degrees(real_electrical_angle), degrees(measured_electrical_angle), degrees(_normalizeAngle(offset_angle)));
    }
    for (; a > destination2; a -= 0.4) {
        motor.move(a);
        delay(100);
        for (uint8_t i = 0; i < 100; i++) {
            encoder.update();
            delay(1);
        }
        float real_electrical_angle = _normalizeAngle(a);
        float measured_electrical_angle = _normalizeAngle( (float)(motor.sensor_direction * measured_pole_pairs) * encoder.getMechanicalAngle()  - 0);

        float offset_angle = measured_electrical_angle - real_electrical_angle;
        offset_x += cosf(offset_angle);
        offset_y += sinf(offset_angle);

        LOG_INFO("%.2f, %.2f, %.2f", degrees(real_electrical_angle), degrees(measured_electrical_angle), degrees(_normalizeAngle(offset_angle)));
    }
    motor.voltage_limit = 0;
    motor.move(a);

    float avg_offset_angle = atan2f(offset_y, offset_x);


    // #### Apply settings
    motor.pole_pairs = measured_pole_pairs;
    motor.zero_electric_angle = avg_offset_angle + _3PI_2;
    motor.voltage_limit = FOC_VOLTAGE_LIMIT;
    motor.controller = MotionControlType::torque;

    LOG_INFO("");
    LOG_INFO("RESULTS:");
    LOG_INFO("  ZERO_ELECTRICAL_OFFSET: %.2f", motor.zero_electric_angle);
    if (motor.sensor_direction == Direction::CW) {
        LOG_INFO("  FOC_DIRECTION: Direction::CW");
    } else {
        LOG_INFO("  FOC_DIRECTION: Direction::CCW");
    }
    LOG_INFO("  MOTOR_POLE_PAIRS: %d", motor.pole_pairs);

    LOG_INFO("");
    LOG_INFO("Saving to persistent configuration...");
    PB_MotorCalibration calibration = {
        .calibrated = true,
        .zero_electrical_offset = motor.zero_electric_angle,
        .direction_cw = motor.sensor_direction == Direction::CW,
        .pole_pairs = motor.pole_pairs,
    };
    if (configuration_.setMotorCalibrationAndSave(calibration)) {
        LOG_SUCCESS("Success!");
    }
}

void MotorTask::checkSensorError() {
#if SENSOR_TLV
    if (encoder.getAndClearError()) {
        LOG_ERROR("LOCKED!");
    }
#elif SENSOR_MT6701
    MT6701Error error = encoder.getAndClearError();
    if (error.error) {
        LOG_ERROR("CRC error. Received %d; calculated %d", error.received_crc, error.calculated_crc);
    }
#endif
}
