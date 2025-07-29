#include "cmsis_os2.h"
#include "elog.h"
#include "gpio.h"
#include "uwb_task.h"

extern void UWB_Task_Init(void);

const char *TAG = "slave_app";

extern "C" int main_app(void) {
    UWB_Task_Init();

    for (;;) {

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(100);
    }
    return 0;
}