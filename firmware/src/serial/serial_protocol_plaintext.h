#pragma once

#include "../proto_gen/smartknob.pb.h"

#include "tasks/motor_task.h"
#include "serial_protocol.h"
#include "uart_stream.h"

typedef std::function<void(void)> DemoConfigChangeCallback;
typedef std::function<void(void)> StrainCalibrationCallback;
typedef std::function<void(void)> MotorCalibrationCallback;

class SerialProtocolPlaintext : public SerialProtocol {
    public:
        SerialProtocolPlaintext(Stream& stream) : SerialProtocol(), stream_(stream) {}
        ~SerialProtocolPlaintext(){}
        void log(const std::string& msg) override;
        void loop() override;
        void handleState(const PB_SmartKnobState& state) override;

        void init(
            DemoConfigChangeCallback demo_config_change_callback
            , StrainCalibrationCallback strain_calibration_callback
            , MotorCalibrationCallback motor_calibration_callback
        );
    
    private:
        Stream& stream_;
        PB_SmartKnobState latest_state_ = {};
        MotorCalibrationCallback motor_calibration_callback_;
        DemoConfigChangeCallback demo_config_change_callback_;
        StrainCalibrationCallback strain_calibration_callback_;
};
