#pragma once

#include "page.h"
#include "util.h"
#include "views/view.h"

#include "tasks/motor_task.h"
#include "tasks/connectivity_task.h"

static constexpr float VOLUME_MIN = 0.0f;
static constexpr float VOLUME_MAX = 0.5f;

static constexpr uint32_t VOLUME_UPDATE_COOLDOWN_MS = 1000; // Cooldown from the last time the lights page published a brightness value, until it will update its own brightness from received MQTT messages
static constexpr uint32_t VOLUME_PUBLISH_FREQUENCY_MS = 500; // Frequency at which the lights page will publish its position to MQTT
static constexpr uint32_t INCOMING_VOLUME_QUEUE_SIZE = 1; // Size of the incoming brightness queue

class VolumePage : public Page {
    public:
        static const uint32_t INCOMING_VOLUME_QUEUE_SIZE = 1; // Size of the incoming brightness queue
    
        VolumePage(PageContext& context
                 , ConnectivityTask &connectivity_task)
            : Page(context)
            , connectivity_task_(connectivity_task)
        {
            incoming_volume_queue_ = xQueueCreate(INCOMING_VOLUME_QUEUE_SIZE, sizeof(BrightnessData));
            assert(incoming_volume_queue_ != NULL);
        }

        ~VolumePage() {
            vQueueDelete(incoming_volume_queue_);
        }

        PB_SmartKnobConfig *getPageConfig() override;
        void                handleState(PB_SmartKnobState state) override;
        void                handleUserInput(input_t input, int input_data, PB_SmartKnobState state) override;

        // QueueHandle_t getIncomingBrightnessQueue() { return incoming_volume_queue_; }

    private:
        ConnectivityTask& connectivity_task_;

        QueueHandle_t incoming_volume_queue_;

        uint32_t last_publish_time_;
        uint32_t last_published_position_;

        PB_SmartKnobConfig config_ =
        {
            .has_view_config = true,
            .view_config =
            {
                VIEW_SLIDER,
                "Volume"
            },
            .initial_position       = 10,
            .sub_position_unit      = 0,
            .position_nonce         = 0,
            .min_position           = 0,
            .max_position           = 50,
            .infinite_scroll        = false,

            // 360*0.75 / 50 = 5.40 degrees per position
            // 360*0.60 / 50 = 4.32 degrees per position
            // .position_width_radians = 2.7 * PI / 180, // 2.7 degrees equals 3/4 of a circle at 100 positions
            .position_width_radians = 5.40 * PI / 180,
            .detent_strength_unit   = 0.4,
            .endstop_strength_unit  = 1,
            .snap_point             = 1.1,
            .detent_positions_count = 0,
            .detent_positions       = {},
            .snap_point_bias        = 0,
            .led_hue                = 30
        };

        // void checkForBrightnessUpdates(PB_SmartKnobState&, PB_SmartKnobConfig&);
};
