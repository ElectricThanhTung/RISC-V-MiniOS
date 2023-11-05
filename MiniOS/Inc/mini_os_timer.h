
#ifndef __MINI_OS_TIMER_H
#define __MINI_OS_TIMER_H

#include <stdint.h>

MiniOS_Task *MiniOS_TimerStart(void (*func)(), uint32_t interval, uint32_t stackSize);
uint8_t MiniOS_TimerStop(MiniOS_Task *timerTask);

#endif /* __MINI_OS_TIMER_H */
