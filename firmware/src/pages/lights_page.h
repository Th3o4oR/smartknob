
#include "page.h"
#include "views/view.h"
#include "../motor_task.h"
#include "../connectivity_task.h"

constexpr unsigned long PAGE_SELECTION_COOLDOWN = 50; // Really only needs to be longer than the motor_task's update interval (5ms as of this comment)

class LightsPage : public Page {
  public:
    LightsPage(ConnectivityTask &connectivity_task)
        : Page()
        , connectivity_task_(connectivity_task) {}

    ~LightsPage() {}

    PB_SmartKnobConfig *getPageConfig() override;
    void                handleState(PB_SmartKnobState state) override;
    void                handleUserInput(input_t input, int input_data, PB_SmartKnobState state) override;

  private:
    ConnectivityTask &connectivity_task_;

    uint32_t last_publish_time;
    uint32_t last_published_position;

    PB_SmartKnobConfig config_ =
      {
        .has_view_config = true,
        .view_config =
          {
                        VIEW_DIAL,
                        "Desk lights"
          },
        .initial_position       = 127,
        .sub_position_unit      = 0,
        .position_nonce         = 6,
        .min_position           = 0,
        .max_position           = 255,
        .infinite_scroll        = false,
        .position_width_radians = 1 * PI / 180,
        .detent_strength_unit   = 0.4,
        .endstop_strength_unit  = 1,
        .snap_point             = 1.1,
        .detent_positions_count = 0,
        .detent_positions       = {},
        .snap_point_bias        = 0,
        .led_hue                = 30
    };
};
