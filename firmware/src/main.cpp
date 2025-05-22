#include <Arduino.h>

#include "configuration.h"
#include "tasks/display_task.h"
#include "tasks/interface_task.h"
#include "tasks/motor_task.h"
#include "tasks/connectivity_task.h"

Configuration config;

#define DISPLAY_TASK_CORE      0
#define MOTOR_TASK_CORE        1
#define CONNECTIVITY_TASK_CORE 0
#define INTERFACE_TASK_CORE    0

// Note that ESP-IDF specifies the stack size in bytes, not words
#define DISPLAY_TASK_STACK_DEPTH      5200
#define MOTOR_TASK_STACK_DEPTH        4500
#define INTERFACE_TASK_STACK_DEPTH    4800
#define CONNECTIVITY_TASK_STACK_DEPTH 4500

/*
    Additional default tasks, from ESP-IDF: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html
    More stack sizes can be found `sdkconfig.h`
    Note that ESP-IDF specifies the stack size in bytes, not words
    - Idle task:           1536 bytes (times two, one for each core)
    - FreeRTOS Timer task: 2048 bytes
    - Main task:           3584 bytes
    - IPC task:            1024 bytes (times two, one for each core)
    - ESP Timer task:      3584 bytes

    Also note that other ESP features, such as WiFi, Bluetooth, etc. may also create tasks with their own stack sizes.
*/

/*
    Stack high water (2025-03-13):
    (after running motor calibration, connecting to WiFi and sending MQTT messages)
    main:         5388 free (can't be changed?)
    display:      4092 free -> (8192-4092) = 4100 used
    motor:         448 free -> (4500- 448) = 4052 used
    interface:    1136 free -> (3600-1136) = 2464 used
    connectivity: 1848 free -> (4096-1848) = 2248 used

    Stack high water (2025-05-21):
    (after running motor calibration, connecting to WiFi and sending MQTT messages)
    display:       972 free -> (5200- 972) = 4228 used
    motor:         476 free -> (4500- 476) = 4024 used
    interface:     588 free -> (4800- 588) = 4212 used
    connectivity: 1008 free -> (4500-1008) = 3492 used
*/

#if SK_DISPLAY
static DisplayTask display_task(DISPLAY_TASK_CORE, DISPLAY_TASK_STACK_DEPTH);
static DisplayTask* display_task_p = &display_task;
#else
static DisplayTask* display_task_p = nullptr;
#endif // SK_DISPLAY
static MotorTask motor_task(MOTOR_TASK_CORE, MOTOR_TASK_STACK_DEPTH, config);
static ConnectivityTask connectivity_task(CONNECTIVITY_TASK_CORE, CONNECTIVITY_TASK_STACK_DEPTH);

InterfaceTask interface_task(INTERFACE_TASK_CORE, INTERFACE_TASK_STACK_DEPTH, motor_task, display_task_p, connectivity_task);

void setup() {
    #if SK_DISPLAY
    display_task.setLogger(&interface_task);
    display_task.begin();

    // Connect display to motor_task's knob state feed
    motor_task.addListener(display_task.getKnobStateQueue());
    #endif // SK_DISPLAY
    
    config.setLogger(&interface_task);
    motor_task.setLogger(&interface_task);
    connectivity_task.setLogger(&interface_task);
    
    config.loadFromDisk();
    interface_task.begin();
    interface_task.setConfiguration(&config);
    motor_task.begin();
    connectivity_task.begin();
    
    // Free up the main loop task (prevents `loop` from running)
    vTaskDelete(NULL); // Note: Main task seems to prevent the connectivity task from connecting to WiFi
    return;
}

void loop() {
    // This function is intentionally left empty.
    // The main loop is handled by FreeRTOS tasks.
    // The main task is deleted in setup().
}
