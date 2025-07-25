#include <stdio.h>

#include "cmsis_os.h"
#include "deca_device_api.h"
#include "deca_regs.h"
#include "elog.h"
#include "port.h"


#define FRAME_LEN_MAX 127

/* Default communication configuration. We use here EVK1000's default mode (mode
 * 3). */
static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC32,       /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_6M8,      /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (1025 + 64 - 32) /* SFD timeout (preamble length + 1 + SFD length - PAC
                        size). Used in RX only. */
};

/* The frame sent in this example is an 802.15.4e standard blink. It is a
 * 12-byte frame composed of the following fields:
 *     - byte 0: frame type (0xC5 for a blink).
 *     - byte 1: sequence number, incremented for each new frame.
 *     - byte 2 -> 9: device ID, see NOTE 1 below.
 *     - byte 10/11: frame check-sum, automatically set by DW1000.  */
static uint8 tx_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 0, 0};
/* Index to access to sequence number of the blink frame in the tx_msg array. */
#define BLINK_FRAME_SN_IDX 1

/* Inter-frame delay period, in milliseconds. */
#define TX_DELAY_MS 1000

static osThreadId_t uwbTaskHandle;

static void uwb_task(void *argument) {
    const char *TAG = "uwb_task";
    reset_DW1000();
    port_set_dw1000_slowrate();
    if (dwt_initialise(DWT_LOADNONE) == DWT_ERROR) {
        elog_e(TAG, "init fail");
        while (1) {
        };
    }
    port_set_dw1000_fastrate();

    /* Configure DW1000. */
    dwt_configure(&config);

    uint32_t device_id = dwt_readdevid();    // 读取DW1000的设备ID
    elog_i(TAG, "DW1000 Device ID: 0x%08X", device_id);

    /* Infinite loop */
    for (;;) {
        int i;

        /* Write frame data to DW1000 and prepare transmission. See NOTE 4
         * below.*/
        dwt_writetxdata(sizeof(tx_msg), tx_msg,
                        0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_msg), 0,
                         0); /* Zero offset in TX buffer, no ranging. */

        /* Start transmission. */
        dwt_starttx(DWT_START_TX_IMMEDIATE);

        /* Poll DW1000 until TX frame sent event set. See NOTE 5 below.
         * STATUS register is 5 bytes long but, as the event we are looking at
         * is in the first byte of the register, we can use this simplest API
         * function to access it.*/
        while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS)) {
        };

        /* Clear TX frame sent event. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

        /* Execute a delay between transmissions. */
        Sleep(TX_DELAY_MS);

        /* Increment the blink frame sequence number (modulo 256). */
        tx_msg[BLINK_FRAME_SN_IDX]++;
        elog_i(TAG, "tx_msg[BLINK_FRAME_SN_IDX] = %d",
               tx_msg[BLINK_FRAME_SN_IDX]);

        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(100);
    }
}

void UWB_Task_Init(void) {
    const osThreadAttr_t uwbTask_attributes = {
        .name = "uwbTask",
        .stack_size = 512 * 4,
        .priority = (osPriority_t)osPriorityNormal,
    };
    uwbTaskHandle = osThreadNew(uwb_task, NULL, &uwbTask_attributes);
}
