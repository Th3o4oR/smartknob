#if SK_DISPLAY

#include "display_task.h"
#include "semaphore_guard.h"
#include "CST816D.h"

#include "views/view.h"
#include "views/styles.h"
#include "views/circle_menu_view.h"
#include "views/list_menu_view.h"
#include "views/dial_view.h"
#include "views/slider_view.h"

static const uint8_t LEDC_CHANNEL_LCD_BACKLIGHT = 0;

TFT_eSPI tft_ = TFT_eSPI();

static lv_disp_draw_buf_t disp_buf;

static DisplayTask* display_task_;

#if SK_TOUCH
CST816D touch(PIN_SDA, PIN_SCL, PIN_TP_RST, PIN_TP_INT);

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{

  bool touched;
  uint8_t gesture;
  uint16_t touchX, touchY;

  SemaphoreGuard lock(*display_task_->i2c_mutex_);
  touched = touch.getTouch(&touchX, &touchY, &gesture);

  if (!touched)
  {
    data->state = LV_INDEV_STATE_REL;
  }
  else
  {
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;
  }
}
#endif // SK_TOUCH

DisplayTask::DisplayTask(const uint8_t task_core, const uint32_t stack_depth) : Task{"Display", stack_depth, 1, task_core} {
  display_task_ = this;
  knob_state_queue_ = xQueueCreate(1, sizeof(PB_SmartKnobState));
  assert(knob_state_queue_ != NULL);

  mutex_ = xSemaphoreCreateMutex();
  assert(mutex_ != NULL);
}

DisplayTask::~DisplayTask() {
  vQueueDelete(knob_state_queue_);
  vSemaphoreDelete(mutex_);
}

void disp_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft_.startWrite();
    tft_.setAddrWindow(area->x1, area->y1, w, h);
    tft_.setSwapBytes(true);
    tft_.pushPixels(&color_p->full, w * h);
    tft_.endWrite();

    lv_disp_flush_ready(disp);
}

void DisplayTask::clear_screen() {
  lv_obj_clean(lv_scr_act());
}

void DisplayTask::button_event_cb(lv_event_t * event)
{
    lv_obj_t * button = lv_event_get_target(event);
    lv_indev_wait_release(lv_indev_get_act());
    int button_index = (int) lv_obj_get_user_data(button);
    display_task_->publish(
      {
        .inputType = INPUT_WITH_DATA,
        .inputData = button_index
      }
    );
}

void gesture_event_cb(lv_event_t * e)
{
  lv_obj_t * screen = lv_event_get_current_target(e);
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
  switch(dir) {
    case LV_DIR_TOP:
      display_task_->publish(
        {
          .inputType = INPUT_BACK,
          .inputData = NULL
        }
      );
      break;
    case LV_DIR_BOTTOM:
      display_task_->publish(
        {
          .inputType = INPUT_FORWARD,
          .inputData = NULL
        }
      );
      break;
    case LV_DIR_RIGHT:
      display_task_->publish(
        {
          .inputType = INPUT_NEXT,
          .inputData = NULL
        }
      );
      break;
    case LV_DIR_LEFT:
      display_task_->publish(
        {
          .inputType = INPUT_PREV,
          .inputData = NULL
        }
      );
      break;
  }
}

void DisplayTask::run() {    
    tft_.begin();
    tft_.invertDisplay(1);
    tft_.setRotation(SK_DISPLAY_ROTATION);
    tft_.fillScreen(TFT_DARKGREEN);

    #if SK_TOUCH
    touch.begin();
    #endif // SK_TOUCH

    ledcSetup(LEDC_CHANNEL_LCD_BACKLIGHT, 5000, SK_BACKLIGHT_BIT_DEPTH); // Set up a signal at 5000 Hz, with the resolution defined in the ini file
    ledcAttachPin(PIN_LCD_BACKLIGHT, LEDC_CHANNEL_LCD_BACKLIGHT);
    ledcWrite(LEDC_CHANNEL_LCD_BACKLIGHT, (1 << SK_BACKLIGHT_BIT_DEPTH) - 1); // Set the duty cycle to max, to show the green initialization screen

    lv_init();
    lv_color_t *buf = (lv_color_t*) heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf);
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);
  
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
      
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    #if SK_TOUCH
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    #endif // SK_TOUCH
  
    screen = lv_scr_act();
    lv_obj_add_event_cb(screen, gesture_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);

    init_styles();

    View * current_view;

    CircleMenuView circleMenuView = CircleMenuView(screen, display_task_);
    ListMenuView listMenuView = ListMenuView(screen, display_task_);
    DialView dialView = DialView(screen, display_task_);
    SliderView sliderView = SliderView(screen, display_task_);

    while(1) {
        PB_SmartKnobState state;
        if (xQueueReceive(knob_state_queue_, &state, portMAX_DELAY) == pdFALSE) {
          continue;
        }

        state_ = state;

        bool config_change = (latest_state_.config.detent_strength_unit != state.config.detent_strength_unit)
        || (latest_state_.config.endstop_strength_unit != state.config.endstop_strength_unit)
        || (latest_state_.config.min_position != state.config.min_position)
        || (latest_state_.config.max_position != state.config.max_position);

        // bool position_change = latest_state_.current_position != state.current_position;

        latest_state_ = state;

        if (config_change) {
          clear_screen();
          switch (state.config.view_config.view_type) {
            case VIEW_SLIDER:
              current_view = &sliderView;
              break;
            case VIEW_DIAL:
              current_view = &dialView;
              break;
            case VIEW_CIRCLE_MENU:
              current_view = &circleMenuView;
              break;
            case VIEW_LIST_MENU:
              current_view = &listMenuView;
              break;
            default:
              LOG_ERROR("Unknown view type: %d", state.config.view_config.view_type);
              assert(0);
          }
          LOG_INFO("Switching to view %d", state.config.view_config.view_type);
          current_view->setupView(state.config);
        }

        current_view->updateView(state);

        lv_task_handler();

        // static uint32_t last_brightness_report;
        // char buf_[100];
        // if (millis() - last_brightness_report > 500) {
        //     snprintf(buf_, sizeof(buf_), "brightness_: %d", brightness_);
        //     log(buf_);
        //     last_brightness_report = millis();
        // }
        {
          SemaphoreGuard lock(mutex_);
          ledcWrite(LEDC_CHANNEL_LCD_BACKLIGHT, brightness_);
        }
        delay(1);
    }
}

QueueHandle_t DisplayTask::getKnobStateQueue() {
  return knob_state_queue_;
}

void DisplayTask::setBrightness(uint16_t brightness) {
  SemaphoreGuard lock(mutex_);
  brightness_ = brightness >> (16 - SK_BACKLIGHT_BIT_DEPTH);
}

void DisplayTask::setListener(QueueHandle_t queue) {
    listener_ = queue;
}

void DisplayTask::publish(userInput_t user_input) {
  xQueueOverwrite(listener_, &user_input);
}

void DisplayTask::setI2CMutex(SemaphoreHandle_t * mutex) {
    i2c_mutex_ = mutex;
}

#endif // SK_DISPLAY
