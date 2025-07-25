// #include "slave_app.h"
#include <stdio.h>
#include "elog.h"

extern void UWB_Task_Init(void);

const char *TAG = "slave_app";

int main_app(void) {
    // elog_i(TAG, "slave_app");
    UWB_Task_Init();
    return 0;
}