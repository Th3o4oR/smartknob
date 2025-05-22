#if SK_LEDS
#include <FastLED.h>
#endif // SK_LEDS

#if SK_STRAIN
#include <HX711.h>
#endif // SK_STRAIN

#if SK_ALS
#include <Adafruit_VEML7700.h>
#endif // SK_ALS

// MOVE THESE TO HEADER WHEN COMPLETED
#include <vector>
#include <map>

#include "interface_task.h"
#include "semaphore_guard.h"
#include "util.h"

#if SK_LEDS
CRGB leds[NUM_LEDS];
#endif // SK_LEDS

#if SK_STRAIN
HX711 scale;
#endif // SK_STRAIN

#if SK_ALS
Adafruit_VEML7700 veml = Adafruit_VEML7700();
#endif // SK_ALS

/**
 * Constructor for the InterfaceTask, responsible for initializing task dependencies
 * and shared resources like queues and mutexes.
 *
 * Note on initialization order:
 * - The EventBus (page_event_bus_) must be explicitly initialized in the member initializer list.
 *   This ensures its internal FreeRTOS queue is created at a safe point in program execution.
 * - In C++, class member variables are constructed in the order they are declared in the class,
 *   *before* the body of the constructor is executed.
 * - If page_event_bus_ is not explicitly initialized in the initializer list, it will be
 *   default-constructed before the task is fully constructed or before the scheduler is ready.
 */
InterfaceTask::InterfaceTask(const uint8_t task_core, const uint32_t stack_depth, MotorTask &motor_task, DisplayTask *display_task, ConnectivityTask &connectivity_task)
    : Task("Interface", stack_depth, 1, task_core)
    , stream_()
    , motor_task_(motor_task)
    , display_task_(display_task)
    , connectivity_task_(connectivity_task)
    , plaintext_protocol_(stream_)
    , proto_protocol_(stream_, [this](PB_SmartKnobConfig &config) { applyConfig(config, true); })
    , page_event_bus_()
    {
#if SK_DISPLAY
    assert(display_task != nullptr);
#endif // SK_DISPLAY

    log_queue_ = xQueueCreate(10, sizeof(std::string *));
    assert(log_queue_ != NULL);

    knob_state_queue_ = xQueueCreate(1, sizeof(PB_SmartKnobState));
    assert(knob_state_queue_ != NULL);

    user_input_queue_ = xQueueCreate(1, sizeof(userInput_t));
    assert(user_input_queue_ != NULL);

    mutex_ = xSemaphoreCreateMutex();
    assert(mutex_ != NULL);

    i2c_mutex_ = xSemaphoreCreateMutex();
    assert(i2c_mutex_ != NULL);
    i2c_mutex = &i2c_mutex_;
    display_task_->setI2CMutex(i2c_mutex);

    auto page_context_ = PageContext {
        .event_bus = page_event_bus_,
        .logger    = this
    };
    page_map_[PageType::MAIN_MENU_PAGE]  = std::make_unique<MainMenuPage>(page_context_);
    page_map_[PageType::SETTINGS_PAGE]   = std::make_unique<SettingsPage>(page_context_);
    page_map_[PageType::MEDIA_MENU_PAGE] = std::make_unique<MediaMenuPage>(page_context_, connectivity_task_);
    page_map_[PageType::VOLUME_PAGE]     = std::make_unique<VolumePage>(page_context_, connectivity_task_);
    page_map_[PageType::MORE_PAGE]       = std::make_unique<MorePage>(page_context_);
    page_map_[PageType::DEMO_PAGE]       = std::make_unique<DemoPage>(page_context_);
    page_map_[PageType::LIGHTS_PAGE]     = std::make_unique<LightsPage>(page_context_, connectivity_task_);

    motor_task_.addListener(knob_state_queue_);
    display_task_->setListener(user_input_queue_);
}

InterfaceTask::~InterfaceTask() {
    vSemaphoreDelete(mutex_);
    vQueueDelete(log_queue_);
    vQueueDelete(knob_state_queue_);
    vQueueDelete(user_input_queue_);
    vSemaphoreDelete(i2c_mutex_);
}

void InterfaceTask::run() {
    stream_.begin();

#if SK_LEDS
    FastLED.addLeds<SK6812, PIN_LED_DATA, GRB>(leds, NUM_LEDS);
#endif // SK_LEDS

#if SK_ALS && PIN_SDA >= 0 && PIN_SCL >= 0
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);
#endif // SK_ALS

#if SK_STRAIN
    scale.begin(PIN_STRAIN_DO, PIN_STRAIN_SCK);
#endif // SK_STRAIN

#if SK_ALS
    if (veml.begin()) {
        veml.setGain(VEML7700_GAIN_2);
        veml.setIntegrationTime(VEML7700_IT_400MS);
    } else {
        LOG_WARN("ALS sensor not found!");
    }
#endif // SK_ALS

    plaintext_protocol_.init(
      [this]() {
          setUserInput(
            {.inputType = INPUT_FORWARD,
             .inputData = NULL},
            true
          );
      },
      [this]() {
          if (!configuration_loaded_) {
              return;
          }
          if (strain_calibration_step_ == 0) {
              LOG_INFO("Strain calibration step 0: Don't touch the knob, then press 'S' again");
              strain_calibration_step_ = 1;
          } else if (strain_calibration_step_ == 1) {
              configuration_value_.strain.idle_value = strain_reading_;
              LOG_INFO("  idle_value=%d", configuration_value_.strain.idle_value);
              LOG_INFO("Strain calibration step 1: Push and hold down the knob with medium pressure, and press 'S' again");
              strain_calibration_step_ = 2;
          } else if (strain_calibration_step_ == 2) {
              configuration_value_.strain.press_delta = strain_reading_ - configuration_value_.strain.idle_value;
              configuration_value_.has_strain         = true;
              LOG_INFO("  press_delta=%d", configuration_value_.strain.press_delta);
              LOG_INFO("Strain calibration complete! Saving...");
              strain_calibration_step_ = 0;
              if (configuration_->setStrainCalibrationAndSave(configuration_value_.strain)) {
                  LOG_SUCCESS("  Saved!");
              } else {
                  LOG_ERROR("  FAILED to save config!");
              }
          }
      },
        [this]() {
            if (!configuration_loaded_) {
                return;
            }
            motor_task_.runCalibration();
        }
    );

    SerialProtocol *current_protocol;
    // Start in legacy protocol mode
    current_protocol_ = &plaintext_protocol_;

    ProtocolChangeCallback protocol_change_callback = [this](uint8_t protocol) {
        switch (protocol) {
        case SERIAL_PROTOCOL_LEGACY:
            current_protocol_ = &plaintext_protocol_;
            break;
        case SERIAL_PROTOCOL_PROTO:
            current_protocol_ = &proto_protocol_;
            break;
        default:
            LOG_ERROR("Unknown protocol requested");
            break;
        }
    };

    plaintext_protocol_.setProtocolChangeCallback(protocol_change_callback);
    proto_protocol_.setProtocolChangeCallback(protocol_change_callback);

    TaskMonitor monitored_tasks[] = {
        {"display", display_task_ ? display_task_->getHandle() : nullptr},
        {"motor", motor_task_.getHandle()},
        {"interface", this->getHandle()},
        {"connectivity", connectivity_task_.getHandle()}
    };
    size_t monitored_tasks_count = sizeof(monitored_tasks) / sizeof(TaskMonitor);

    // Set initial page
    changePage(PageType::MAIN_MENU_PAGE);

    // Interface loop:
    while (1) {
        PB_SmartKnobState new_state;
        if (xQueueReceive(knob_state_queue_, &new_state, 0) == pdTRUE) {
            // Discard all outdated state messages (incorrect nonce)
            // if (true) {
            if (new_state.config.position_nonce == position_nonce_) {
                latest_state_ = new_state;
                publishState();
                current_page_->handleState(latest_state_);
            } else {
                LOG_WARN("Discarding outdated state message (expected nonce %d, got %d)", position_nonce_, new_state.config.position_nonce);
            }
        }

        static uint32_t last_stack_log = 0;
        if (millis() - last_stack_log > 1000) {
            last_stack_log = millis();
            monitorStackAndHeapUsage(monitored_tasks, monitored_tasks_count);
            // logStackAndHeapUsage(monitored_tasks, monitored_tasks_count);
        }

        PageEvent::Message event;
        if (page_event_bus_.poll(event)) {
            auto visitor = overload {
                [&](const PageEvent::PageChange& e) {
                    changePage(e.new_page);
                },
                [&](PageEvent::ConfigChange& e) {
                    applyConfig(e.config, false);
                },
                [&](const PageEvent::MotorCalibration&) {
                    motor_task_.runCalibration();
                }
            };
            std::visit(visitor, event);
        }

        current_protocol_->loop();

        userInput_t user_input;
        if (xQueueReceive(user_input_queue_, &user_input, 0) == pdTRUE) {
            setUserInput(user_input, true);
        }

        if (user_input_.inputType > -1) {
            current_page_->handleUserInput(user_input_.inputType, user_input_.inputData, latest_state_);
            user_input_.inputType = (input_t)-1;
            user_input_.inputData = NULL;
        }

        std::string *log_string;
        while (xQueueReceive(log_queue_, &log_string, 0) == pdTRUE) {
            current_protocol_->log(log_string->c_str());
            delete log_string;
        }

        updateHardware();

        if (!configuration_loaded_) {
            SemaphoreGuard lock(mutex_);
            if (configuration_ != nullptr) {
                configuration_value_  = configuration_->get();
                configuration_loaded_ = true;
            }
        }

        delay(1);
    }
}

void InterfaceTask::setUserInput(userInput_t user_input, bool playHapticts) {
    user_input_ = user_input;
    if (playHapticts) {
        motor_task_.playHaptic(true);
    }
}

void InterfaceTask::log(const std::string &msg) {
    // Allocate a string for the duration it's in the queue; it is free'd by the queue consumer
    std::string *msg_str = new std::string(msg);

    // Put string in queue (or drop if full to avoid blocking)
    xQueueSendToBack(log_queue_, &msg_str, 0);
}

void InterfaceTask::updateHardware() {
    // How far button is pressed, in range [0, 1]
    float press_value_unit = 0;

#if SK_ALS
    SemaphoreGuard lock(i2c_mutex_);
    const float    LUX_ALPHA = 0.005;
    static float   lux_avg;
    float          lux = veml.readLux();
    lux_avg            = lux * LUX_ALPHA + lux_avg * (1 - LUX_ALPHA);
    // static uint32_t last_als;
    // if (millis() - last_als > 1000 && strain_calibration_step_ == 0) {
    //     LOG_INFO("millilux: %.2f", lux*1000);
    //     last_als = millis();
    // }
#endif // SK_ALS

    static bool pressed;
#if SK_STRAIN
    if (scale.wait_ready_timeout(100)) {
        strain_reading_ = scale.read();

        // static uint32_t last_reading_display;
        // if (millis() - last_reading_display > 100 && strain_calibration_step_ == 0) {
        //     LOG_INFO("HX711 reading: %d", strain_reading_);
        //     last_reading_display = millis();
        // }

        if (configuration_loaded_ && configuration_value_.has_strain && strain_calibration_step_ == 0) {
            // TODO: calibrate and track (long term moving average) idle point (lower)
            press_value_unit = mapf(strain_reading_, configuration_value_.strain.idle_value, configuration_value_.strain.idle_value + configuration_value_.strain.press_delta, 0, 1);

            // Ignore readings that are way out of expected bounds
            if (-1 < press_value_unit && press_value_unit < 2) {
                static uint8_t press_readings;
                if (!pressed && press_value_unit > 1) {
                    press_readings++;
                    if (press_readings > 2) {
                        motor_task_.playHaptic(true);
                        pressed = true;
                        press_count_++;
                        publishState();
                        setUserInput(
                          {.inputType = INPUT_FORWARD,
                           .inputData = NULL},
                          false
                        );
                    }
                } else if (pressed && press_value_unit < 0.5) {
                    press_readings++;
                    if (press_readings > 2) {
                        motor_task_.playHaptic(false);
                        pressed = false;
                    }
                } else {
                    press_readings = 0;
                }
            }
        }
    } else {
        LOG_WARN("HX711 not found (not ready?)");

#if SK_LEDS
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            leds[i] = CRGB::Red;
        }
        FastLED.show();
#endif // SK_LEDS
    }
#endif // SK_STRAIN

    uint16_t brightness = UINT16_MAX;
// TODO: brightness scale factor should be configurable (depends on reflectivity of surface)
#if SK_ALS
    brightness = (uint16_t)CLAMP(lux_avg * 13000, (float)1280, (float)UINT16_MAX);
#endif // SK_ALS

#if SK_DISPLAY
    display_task_->setBrightness(brightness); // TODO: apply gamma correction
#endif                                        // SK_DISPLAY

#if SK_LEDS
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        leds[i].setHSV(latest_config_.led_hue, 255 - 180 * CLAMP(press_value_unit, (float)0, (float)1) - 75 * pressed, brightness >> 8);

        // Gamma adjustment
        leds[i].r = dim8_video(leds[i].r);
        leds[i].g = dim8_video(leds[i].g);
        leds[i].b = dim8_video(leds[i].b);
    }
    FastLED.show();
#endif // SK_LEDS
}

void InterfaceTask::setConfiguration(Configuration *configuration) {
    SemaphoreGuard lock(mutex_);
    configuration_ = configuration;
}

void InterfaceTask::publishState() {
    // Apply local state before publishing to serial
    latest_state_.press_nonce = press_count_;
    current_protocol_->handleState(latest_state_);
}

uint8_t InterfaceTask::incrementPositionNonce() {
    // SemaphoreGuard lock(mutex_); // TODO: Should this be locked?
    position_nonce_++;
    return position_nonce_;
}

/**
 * @brief Change the current page to the requested page.
 *
 * Used to set relevant parameters on the outgoing and incoming pages,
 * like remembering the last position.
 *
 * @param page The requested page
 */
void InterfaceTask::changePage(PageType page) {
    auto it = this->page_map_.find(page);
    if (it == this->page_map_.end()) {
        LOG_ERROR("Unknown page requested");
        return;
    }

    if (this->current_page_) {
        auto current_config              = this->current_page_->getPageConfig();
        current_config->initial_position = latest_state_.current_position;
    }

    this->current_page_ = it->second.get();
    auto config  = this->current_page_->getPageConfig();
    applyConfig(*config, false);

    const char *page_name = config->has_view_config ? config->view_config.description : "Unnamed";
    LOG_INFO("Switching to page [%s]", page_name);
};

void InterfaceTask::applyConfig(PB_SmartKnobConfig &config, bool from_remote) {
    // Generate a new nonce for the updated state
    config.position_nonce = incrementPositionNonce();

    remote_controlled_ = from_remote;
    latest_config_     = config;
    motor_task_.setConfig(config);
}

/**
 * @brief Monitor the stack and heap usage of the monitored tasks.
 *
 * This function checks the stack and heap usage of the monitored tasks
 * and logs a warning if any task's stack usage is below a certain threshold.
 *
 * @param tasks The array of TaskMonitor structures containing task information.
 * @param count The number of tasks in the array.
 */
void InterfaceTask::monitorStackAndHeapUsage(const TaskMonitor* tasks, size_t count) {
    constexpr unsigned int STACK_WARN_THRESHOLD = 512;  // in bytes

    for (size_t i = 0; i < count; ++i) {
        const auto& task = tasks[i];
        const char* name = task.name;
        TaskHandle_t handle = task.handle;

        if (handle == nullptr) {
            continue;
        }

        uint32_t high_water = uxTaskGetStackHighWaterMark(handle); // in bytes

        // Log warning individually if too low
        if (high_water < STACK_WARN_THRESHOLD) {
            LOG_WARN("Task '%s' low stack! Free: %d words (%d bytes)",
                    name, high_water, high_water * 4);
        }
    }
}
/**
 * @brief Log the stack and heap usage of the monitored tasks.
 *
 * This function logs the stack and heap usage of the monitored tasks
 * to the serial output. It also logs a warning if any task's stack
 * usage is below a certain threshold.
 *
 * @param tasks The array of TaskMonitor structures containing task information.
 * @param count The number of tasks in the array.
 */
void InterfaceTask::logStackAndHeapUsage(const TaskMonitor* tasks, size_t count) {
    constexpr unsigned int STACK_WARN_THRESHOLD = 512;  // in bytes
    constexpr size_t LOG_BUFFER_SIZE = 512;

    char line[LOG_BUFFER_SIZE] = "Stack: ";
    for (size_t i = 0; i < count; ++i) {
        const auto& task = tasks[i];
        const char* name = task.name;
        TaskHandle_t handle = task.handle;

        if (handle == nullptr) {
            snprintf(line + strlen(line), LOG_BUFFER_SIZE - strlen(line), "%s:N/A ", name);
            continue;
        }

        uint32_t high_water = uxTaskGetStackHighWaterMark(handle); // in bytes
        snprintf(line + strlen(line), LOG_BUFFER_SIZE - strlen(line),
                "-- %s: %4d ", name, high_water);
    }

    LOG_INFO("%s", line);
    LOG_INFO("Heap: free: %d bytes, min ever: %d bytes",
            xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
}
