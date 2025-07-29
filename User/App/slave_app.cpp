#include "ContinuityCollector.h"
#include "cmsis_os2.h"
#include "elog.h"
#include "gpio.h"
#include "uwb_task.h"

extern void UWB_Task_Init(void);

const char *TAG = "slave_app";

extern "C" int main_app(void) {
    UWB_Task_Init();

    // Initialize continuity collector with virtual GPIO
    std::unique_ptr<ContinuityCollector> continuityCollector =
        ContinuityCollectorFactory::create();

    CollectorConfig currentConfig = CollectorConfig(2, 0, 4, 20, false);
    continuityCollector->configure(currentConfig);

    std::vector<uint8_t> continuityData;

    for (;;) {
        continuityCollector->startCollection();
        continuityData = continuityCollector->getDataVector();
        elog_v(TAG, "Continuity data length: %d", continuityData.size());
        UWB_SendData(continuityData.data(), continuityData.size(), 0);
        
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(500);
    }
    return 0;
}