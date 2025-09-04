#include "MasterComm.h"
#include "adc.h"
#include "bootloader_flag.h"
#include "cmsis_os2.h"
#include "elog.h"
#include "slave_device.h"
const char *TAG = "slave_app";

using namespace SlaveApp;

// Static pointer to the global SlaveDevice instance
static SlaveDevice *g_GlobalSlaveDevice = nullptr;

// C wrapper function to get synchronized timestamp from SlaveDevice
extern "C" uint32_t SlaveDeviceGetSyncTimestampMs(void *device)
{
    if (device != nullptr)
    {
        auto *slaveDevice = static_cast<SlaveDevice *>(device);
        return slaveDevice->GetSyncTimestampMs();
    }
    return 0;
}

extern "C" int main_app(void)
{
    SlaveDevice slaveDevice;

    // Register SlaveDevice with easylogger for synchronized timestamps
    g_GlobalSlaveDevice = &slaveDevice;
    ElogSetSlaveDevice(&slaveDevice, SlaveDeviceGetSyncTimestampMs);

    elog_i(TAG, "SlaveDevice registered with easylogger for synchronized timestamps");

    slaveDevice.run();

    for (;;)
    {
        osDelay(100);
    }
}