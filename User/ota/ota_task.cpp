#include "ota_task.h"

#include "elog.h"

OtaTask::OtaTask() : TaskClassS("OtaTask", static_cast<TaskPriority>(TASK_PRIORITY_OTA))
{
}

void OtaTask::task()
{
    elog_d(TAG, "OTA task started");

    for (;;)
    {
        // 处理OTA相关逻辑
        processOta();

        // 延时，避免CPU占用过高
        TaskBase::delay(PROCESS_INTERVAL_MS);
    }
}

void OtaTask::processOta() const
{
    // TODO: 实现OTA处理逻辑
    // - 检查是否有待处理的OTA请求
    // - 接收OTA数据
    // - 验证固件完整性
    // - 执行固件更新
}
