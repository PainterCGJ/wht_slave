#include "uwb_ltlp_queue.h"
#include "elog.h"

#define QUEUE_SIZE 1024

static osMessageQueueId_t g_uwb_to_ltlp_queue = NULL;
static osMessageQueueId_t g_ltlp_to_uwb_queue = NULL;
static osEventFlagsId_t g_conducting_event = NULL;

int uwb_ltlp_queue_init(void)
{
    static const char *TAG = "uwb_ltlp_queue";

    // 创建UWB->LTLP队列
    g_uwb_to_ltlp_queue = osMessageQueueNew(QUEUE_SIZE, sizeof(uint8_t), NULL);
    if (g_uwb_to_ltlp_queue == NULL)
    {
        elog_e(TAG, "Failed to create UWB->LTLP queue");
        return -1;
    }

    // 创建LTLP->UWB队列
    g_ltlp_to_uwb_queue = osMessageQueueNew(QUEUE_SIZE, sizeof(uint8_t), NULL);
    if (g_ltlp_to_uwb_queue == NULL)
    {
        elog_e(TAG, "Failed to create LTLP->UWB queue");
        // 清理已创建的队列
        if (g_uwb_to_ltlp_queue != NULL)
        {
            osMessageQueueDelete(g_uwb_to_ltlp_queue);
            g_uwb_to_ltlp_queue = NULL;
        }
        return -1;
    }

    // 创建导通检测状态事件组
    g_conducting_event = osEventFlagsNew(NULL);
    if (g_conducting_event == NULL)
    {
        elog_e(TAG, "Failed to create conducting event flags");
        // 清理已创建的队列
        if (g_uwb_to_ltlp_queue != NULL)
        {
            osMessageQueueDelete(g_uwb_to_ltlp_queue);
            g_uwb_to_ltlp_queue = NULL;
        }
        if (g_ltlp_to_uwb_queue != NULL)
        {
            osMessageQueueDelete(g_ltlp_to_uwb_queue);
            g_ltlp_to_uwb_queue = NULL;
        }
        return -1;
    }

    elog_i(TAG, "UWB-LTLP queues and event flags initialized successfully");
    return 0;
}

osMessageQueueId_t uwb_ltlp_get_uwb_to_ltlp_queue(void)
{
    return g_uwb_to_ltlp_queue;
}

osMessageQueueId_t uwb_ltlp_get_ltlp_to_uwb_queue(void)
{
    return g_ltlp_to_uwb_queue;
}

void uwb_ltlp_set_conducting_state(bool is_conducting)
{
    if (g_conducting_event == NULL)
    {
        return;
    }

    if (is_conducting)
    {
        // 设置导通检测状态事件标志
        osEventFlagsSet(g_conducting_event, UWB_LTLP_EVENT_CONDUCTING);
    }
    else
    {
        // 清除导通检测状态事件标志
        osEventFlagsClear(g_conducting_event, UWB_LTLP_EVENT_CONDUCTING);
    }
}

bool uwb_ltlp_is_conducting(void)
{
    if (g_conducting_event == NULL)
    {
        return false;
    }

    // 检查导通检测状态事件标志是否被设置
    uint32_t flags = osEventFlagsGet(g_conducting_event);
    return (flags & UWB_LTLP_EVENT_CONDUCTING) != 0;
}

void uwb_ltlp_set_ltlp_data_ready(void)
{
    if (g_conducting_event == NULL)
    {
        return;
    }

    // 设置LTLP数据就绪事件标志
    osEventFlagsSet(g_conducting_event, UWB_LTLP_EVENT_LTLP_DATA_READY);
}

void uwb_ltlp_clear_ltlp_data_ready(void)
{
    if (g_conducting_event == NULL)
    {
        return;
    }

    // 清除LTLP数据就绪事件标志
    osEventFlagsClear(g_conducting_event, UWB_LTLP_EVENT_LTLP_DATA_READY);
}

osEventFlagsId_t uwb_ltlp_get_event_flags(void)
{
    return g_conducting_event;
}

