#pragma once

#include <Arduino.h>
#include <SimpleFOC.h>
#include <vector>
#include <variant>

#include "configuration.h"
#include "logger.h"
#include "proto_gen/smartknob.pb.h"
#include "task.h"
#include "event_bus.h"

// Motor event definitions
namespace MotorCommand {
    struct Calibrate {};
    struct SetConfig {
        PB_SmartKnobConfig config;
    };
    struct PlayHaptic {
        bool press;
    };

    using Message = std::variant<
        Calibrate
        , SetConfig
        , PlayHaptic
    >;
    
    static_assert(std::is_pod_v<Calibrate>);
    static_assert(std::is_pod_v<SetConfig>);
    static_assert(std::is_pod_v<PlayHaptic>);
}

class MotorTask : public Task<MotorTask> {
    friend class Task<MotorTask>; // Allow base Task to invoke protected run()

    public:
        MotorTask(const uint8_t task_core, const uint32_t stack_depth, Configuration& configuration);
        ~MotorTask();

        void setConfig(const PB_SmartKnobConfig& config);
        void playHaptic(bool press);
        void runCalibration();

        void addListener(QueueHandle_t queue);

    protected:
        void run();

    private:
        Configuration& configuration_;
        std::vector<QueueHandle_t> listeners_;
        EventBus<MotorCommand::Message> command_bus_;

        // BLDC motor & driver instance
        BLDCMotor motor_ = BLDCMotor(1);
        BLDCDriver6PWM motor_driver_ = BLDCDriver6PWM(PIN_UH, PIN_UL, PIN_VH, PIN_VL, PIN_WH, PIN_WL);

        void publish(const PB_SmartKnobState& state);
        void calibrate();
        void checkSensorError();
};
