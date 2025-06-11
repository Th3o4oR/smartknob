#pragma once

#include "page.h"
#include "util.h"
#include "views/view.h"

#include "tasks/motor_task.h"
#include "tasks/connectivity_task.h"
#include "tasks/interface_task.h"

static constexpr uint8_t BRIGHTNESS_MIN = 0;
static constexpr uint8_t BRIGHTNESS_MAX = 255;

static constexpr uint32_t BRIGHTNESS_UPDATE_COOLDOWN_MS = 1000; // Cooldown from the last time the lights page published a brightness value, until it will update its own brightness from received MQTT messages
static constexpr uint32_t BRIGHTNESS_PUBLISH_FREQUENCY_MS = 500; // Frequency at which the lights page will publish its position to MQTT
static constexpr uint32_t INCOMING_BRIGHTNESS_QUEUE_SIZE = 1; // Size of the incoming brightness queue

class LightsPage : public Page {
    public:
        LightsPage(
                PageContext& context,
                ConnectivityTask& connectivity_task,
                const int16_t fastled_hue,
                const char* trigger_name,
                const char* description
            )
            : Page(context)
            , connectivity_task_(connectivity_task)
        {
            incoming_brightness_queue_ = xQueueCreate(INCOMING_BRIGHTNESS_QUEUE_SIZE, sizeof(BrightnessData));
            assert(incoming_brightness_queue_ != NULL);

            connectivity_task_.registerListener(MQTTSubscriptionType::LIGHTING, incoming_brightness_queue_);

            config_.led_hue = fastled_hue;
            snprintf(trigger_name_.data(), trigger_name_.size(), "%s", trigger_name);
            snprintf(config_.view_config.description, 41, "%s", description); // TODO: Use std::array<char, 41> for description in PB_SmartKnobConfig
        }

        ~LightsPage() {
            vQueueDelete(incoming_brightness_queue_);
        }

        PB_SmartKnobConfig *getPageConfig() override;
        void                handleState(PB_SmartKnobState state) override;
        void                handleUserInput(input_t input, int input_data, PB_SmartKnobState state) override;

        QueueHandle_t getIncomingBrightnessQueue() { return incoming_brightness_queue_; }

    private:
        std::array<char, 10> trigger_name_;

        ConnectivityTask &connectivity_task_;

        QueueHandle_t incoming_brightness_queue_;

        uint32_t last_publish_time_;
        uint32_t last_published_position_;

        PB_SmartKnobConfig config_ =
        {
            .has_view_config = true,
            .view_config =
            {
                VIEW_DIAL,
                "Lights" // Will be overwritten by the constructor
            },
            .initial_position       = 50,
            .sub_position_unit      = 0,
            .position_nonce         = 0,
            .min_position           = 0,
            .max_position           = 100,
            .infinite_scroll        = false,
            .position_width_radians = 2.4 * PI / 180, // How many degrees the knob should turn for each position. At 1 degree, 0-100 means the know will have a range of 100 degrees
            .detent_strength_unit   = 0.4,
            .endstop_strength_unit  = 1,
            .snap_point             = 1.1,
            .detent_positions_count = 0,
            .detent_positions       = {},
            .snap_point_bias        = 0,
            .led_hue                = 30 // Will be overwritten by the constructor
        };

        void checkForBrightnessUpdates(PB_SmartKnobState&, PB_SmartKnobConfig&);
};
