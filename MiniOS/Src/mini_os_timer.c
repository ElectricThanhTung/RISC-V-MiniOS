
#include "mini_os_task.h"

static void MiniOS_TimerTask(void (*func)(), uint32_t interval) {
    while(1) {
        MiniOS_TaskSleep(interval);
        func();
    }
}

MiniOS_Task *MiniOS_TimerStart(void (*func)(), uint32_t interval, uint32_t stackSize) {
    return MiniOS_TaskCreate((void (*)(void *))MiniOS_TimerTask, "Timer", stackSize, 2, func, interval);
}

uint8_t MiniOS_TimerStop(MiniOS_Task *timerTask) {
    return MiniOS_TaskDelete(timerTask);
}
