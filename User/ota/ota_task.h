#pragma once

#include "TaskCPP.h"
#include "config.h"
#include "ltlp_api.h"
#include "msg_def.h"

/**
 * @brief OTA任务类 - 处理固件升级相关任务
 *
 * 负责处理OTA（Over-The-Air）固件升级流程，包括：
 * - 接收升级数据
 * - 验证固件完整性
 * - 执行固件更新
 */
class OtaTask final : public TaskClassS<TASK_STACK_SIZE_OTA>
{
  public:
    /**
     * @brief 构造函数
     */
    OtaTask();

    /**
     * @brief 析构函数
     */
    ~OtaTask() = default;

  private:
    uint8_t m_ltlpReceiverBuffer[1024];
    /**
     * @brief 任务主循环
     *
     * 实现TaskClassS的纯虚函数，作为任务的主执行函数
     */
    void task() override;

    /**
     * @brief 处理OTA相关逻辑
     */
    void processOta() const;

    /**
     * @brief 日志标签
     */
    static constexpr const char TAG[] = "OtaTask";

    /**
     * @brief OTA处理间隔（毫秒）
     */
    static constexpr uint32_t PROCESS_INTERVAL_MS = 100;

    static void onLtltpRecvOneFrame(LtlpFrame *pFrame, void *usrParm);
};
