#pragma once

#include <AceButton.h>
#include <Arduino.h>

#include "configuration.h"
#include "display_task.h"
#include "connectivity_task.h"
#include "logger.h"
#include "motor_task.h"
#include "serial/serial_protocol_plaintext.h"
#include "serial/serial_protocol_protobuf.h"
#include "serial/uart_stream.h"
#include "task.h"

#include "input_type.h"
#include "pages/main_menu_page.h"
#include "pages/more_menu_page.h"
#include "pages/lights_page.h"
#include "pages/demo_page.h"
#include "pages/settings_page.h"
#include "pages/media_control_page.h"
#include "pages/volume_page.h"

#ifndef SK_FORCE_UART_STREAM
    #define SK_FORCE_UART_STREAM 0
#endif // SK_FORCE_UART_STREAM

class InterfaceTask : public Task<InterfaceTask>, public Logger {
    friend class Task<InterfaceTask>; // Allow base Task to invoke protected run()

    public:
        InterfaceTask(const uint8_t task_core, const uint32_t stack_depth, MotorTask& motor_task, DisplayTask* display_task, ConnectivityTask& connectivity_task);
        virtual ~InterfaceTask();

        SemaphoreHandle_t * i2c_mutex;

        void log(const std::string& msg) override;
        void setConfiguration(Configuration* configuration);
        void changePage(PageType page);
        uint8_t incrementPositionNonce();

    protected:
        void run();

    private:
    #if defined(CONFIG_IDF_TARGET_ESP32S3) && !SK_FORCE_UART_STREAM
        HWCDC stream_;
    #else
        UartStream stream_;
    #endif // CONFIG_IDF_TARGET_ESP32S3
        MotorTask& motor_task_;
        DisplayTask* display_task_;
        ConnectivityTask& connectivity_task_;

        SemaphoreHandle_t mutex_;
        SemaphoreHandle_t i2c_mutex_;
        Configuration* configuration_ = nullptr; // protected by mutex_

        PB_PersistentConfiguration configuration_value_;
        bool configuration_loaded_ = false;

        uint8_t strain_calibration_step_ = 0;
        int32_t strain_reading_ = 0;

        SerialProtocol* current_protocol_ = nullptr;
        bool remote_controlled_ = false;
        uint8_t press_count_ = 1;

        uint8_t position_nonce_ = 0; // This will overwrite all position_nonce values defined in the config, but should work fine, since it achieves the same thing

        PB_SmartKnobState latest_state_ = {};
        PB_SmartKnobConfig latest_config_ = {};

        QueueHandle_t log_queue_;
        QueueHandle_t knob_state_queue_;
        QueueHandle_t user_input_queue_;
        SerialProtocolPlaintext plaintext_protocol_;
        SerialProtocolProtobuf proto_protocol_;

        std::map<PageType, std::unique_ptr<Page>> page_map_;
        Page* current_page_ = nullptr;

        userInput_t user_input_;

        void setUserInput(userInput_t user_input, bool playHapticts);
        void updateHardware();
        void publishState();
        void applyConfig(PB_SmartKnobConfig& config, bool from_remote);
};
