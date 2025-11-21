#ifndef UWB_LTLP_QUEUE_H
#define UWB_LTLP_QUEUE_H

#include "cmsis_os2.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief 导通检测状态事件标志位
 */
#define UWB_LTLP_EVENT_CONDUCTING (1UL << 0)

/**
 * @brief LTLP有数据向UWB发送事件标志位
 */
#define UWB_LTLP_EVENT_LTLP_DATA_READY (1UL << 1)

    /**
     * @brief 初始化UWB和LTLP之间的通信队列
     * @return 0表示成功，-1表示失败
     */
    int uwb_ltlp_queue_init(void);

    /**
     * @brief 获取UWB->LTLP队列句柄
     * @return 队列句柄，如果未初始化则返回NULL
     */
    osMessageQueueId_t uwb_ltlp_get_uwb_to_ltlp_queue(void);

    /**
     * @brief 获取LTLP->UWB队列句柄
     * @return 队列句柄，如果未初始化则返回NULL
     */
    osMessageQueueId_t uwb_ltlp_get_ltlp_to_uwb_queue(void);

    /**
     * @brief 设置导通检测状态标志
     * @param is_conducting true表示处于导通检测状态，false表示不在导通检测状态
     */
    void uwb_ltlp_set_conducting_state(bool is_conducting);

    /**
     * @brief 获取导通检测状态标志
     * @return true表示处于导通检测状态，false表示不在导通检测状态
     */
    bool uwb_ltlp_is_conducting(void);

    /**
     * @brief 设置LTLP数据就绪标志（LTLP有数据向UWB发送）
     */
    void uwb_ltlp_set_ltlp_data_ready(void);

    /**
     * @brief 清除LTLP数据就绪标志
     */
    void uwb_ltlp_clear_ltlp_data_ready(void);

    /**
     * @brief 获取事件组句柄（用于等待事件）
     * @return 事件组句柄，如果未初始化则返回NULL
     */
    osEventFlagsId_t uwb_ltlp_get_event_flags(void);

#ifdef __cplusplus
}
#endif

#endif /* UWB_LTLP_QUEUE_H */
