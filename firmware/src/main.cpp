#include <Arduino.h>

#include "configuration.h"
#include "display_task.h"
#include "interface_task.h"
#include "motor_task.h"
#include "connectivity_task.h"

Configuration config;

#define DISPLAY_TASK_STACK_DEPTH      8192
#define MOTOR_TASK_STACK_DEPTH        4500
#define CONNECTIVITY_TASK_STACK_DEPTH 4096
#define INTERFACE_TASK_STACK_DEPTH    3600

/*
    Stack high water (2025-03-13):
    (after running motor calibration, connecting to WiFi and sending MQTT messages)
    main:         5388 free (can't be changed?)
    display:      4092 free -> (8192-4092) = 4100 used
    motor:         448 free -> (4500- 448) = 4052 used
    interface:    1136 free -> (3600-1136) = 2464 used
    connectivity: 1848 free -> (4096-1848) = 2248 used
*/

#if SK_DISPLAY
static DisplayTask display_task(0, DISPLAY_TASK_STACK_DEPTH);
static DisplayTask* display_task_p = &display_task;
#else
static DisplayTask* display_task_p = nullptr;
#endif
static MotorTask motor_task(1, MOTOR_TASK_STACK_DEPTH, config);
static ConnectivityTask connectivity_task(0, CONNECTIVITY_TASK_STACK_DEPTH);

InterfaceTask interface_task(0, INTERFACE_TASK_STACK_DEPTH, motor_task, display_task_p, connectivity_task);

void setup() {
    #if SK_DISPLAY
    display_task.setLogger(&interface_task);
    display_task.begin();

    // Connect display to motor_task's knob state feed
    motor_task.addListener(display_task.getKnobStateQueue());
    #endif

    interface_task.begin();

    config.setLogger(&interface_task);
    config.loadFromDisk();

    interface_task.setConfiguration(&config);

    motor_task.setLogger(&interface_task);
    motor_task.begin();

    connectivity_task.setLogger(&interface_task);
    connectivity_task.begin();

    // Free up the Arduino loop task
    vTaskDelete(NULL);
}

void loop() {
    char buf[256];
    static uint32_t last_stack_debug;
    if (millis() - last_stack_debug > 1000) {
        unsigned int main_high_water = uxTaskGetStackHighWaterMark(NULL);
        unsigned int display_high_water = uxTaskGetStackHighWaterMark(display_task.getHandle());
        unsigned int motor_high_water = uxTaskGetStackHighWaterMark(motor_task.getHandle());
        unsigned int interface_high_water = uxTaskGetStackHighWaterMark(interface_task.getHandle());
        unsigned int connectivity_high_water = uxTaskGetStackHighWaterMark(connectivity_task.getHandle());

        snprintf(buf, sizeof(buf),
            "Stack high water: main: % 4d"
            ", display: % 4d"
            ", motor: % 4d"
            ", interface: % 4d"
            ", connectivity: % 4d",
            main_high_water,
            display_high_water,
            motor_high_water,
            interface_high_water,
            connectivity_high_water
        );
        interface_task.log(buf);

        snprintf(buf, sizeof(buf), "Heap -- free: %d, largest: %d", heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        interface_task.log(buf);
        last_stack_debug = millis();
    }
}
