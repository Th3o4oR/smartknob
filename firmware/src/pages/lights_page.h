#include "page.h"
#include "views/view.h"
#include "tasks/motor_task.h"
#include "tasks/connectivity_task.h"

/**
 * @brief Callback to handle page changes
 * 
 * @param config The new config
 */
typedef std::function<void(PB_SmartKnobConfig *)> ConfigChangeCallback;

static constexpr uint32_t BRIGHTNESS_UPDATE_COOLDOWN_MS = 1000; // Cooldown from the last time the lights page published a brightness value, until it will update its own brightness from received MQTT messages
static constexpr uint32_t MQTT_PUBLISH_FREQUENCY_MS = 500; // Frequency at which the lights page will publish its position to MQTT
static constexpr uint32_t INCOMING_LIGHTING_QUEUE_SIZE = 1; // Size of the incoming brightness queue

class LightsPage : public Page {
    public:
        LightsPage(ConnectivityTask &connectivity_task)
            : Page()
            , connectivity_task_(connectivity_task)
            , incoming_lighting_queue_(xQueueCreate(INCOMING_LIGHTING_QUEUE_SIZE, sizeof(LightingPayload)))
        {
            assert(incoming_lighting_queue_ != NULL);
        }

        ~LightsPage() {
            vQueueDelete(incoming_lighting_queue_);
        }

        PB_SmartKnobConfig *getPageConfig() override;
        void                handleState(PB_SmartKnobState state) override;
        void                handleUserInput(input_t input, int input_data, PB_SmartKnobState state) override;

        QueueHandle_t getIncomingLightingQueue() { return incoming_lighting_queue_; }

        void setConfigChangeCallback(ConfigChangeCallback callback) {
            config_change_callback_ = callback;
        }

    private:
        ConnectivityTask& connectivity_task_;

        ConfigChangeCallback config_change_callback_;

        QueueHandle_t incoming_lighting_queue_;

        uint32_t last_publish_time_;
        uint32_t last_published_position_;

        PB_SmartKnobConfig config_ =
        {
            .has_view_config = true,
            .view_config =
            {
                VIEW_DIAL,
                "Bedroom lights"
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
            .led_hue                = 30
        };

        void checkForLightingUpdates(PB_SmartKnobState&, PB_SmartKnobConfig&);
};
