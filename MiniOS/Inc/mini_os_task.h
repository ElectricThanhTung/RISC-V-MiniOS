
#ifndef __MINI_OS_TASK_H
#define __MINI_OS_TASK_H

#include <stdint.h>

#define SYSTICK_CYCLE           1000 * 6
#define IDLE_STACK_SIZE         128

#define __weak                  __attribute__((__weak__))
#define __interrupt             __attribute__((interrupt()))
#define __optimize_ofast        __attribute__((optimize("Ofast")))

typedef struct {
    const void * const Next;
    const char * const Name;
} MiniOS_Task;

uint32_t MiniOS_GetTickMs(void);
uint32_t MiniOS_GetTickUs(void);
void MiniOS_TaskSwitch(void);
void MiniOS_TaskSuspendAll(void);
void MiniOS_TaskResumeAll(void);
__weak void MiniOS_SysTickUserHandler(void);
__weak void MiniOS_TaskIdle(void);
MiniOS_Task * MiniOS_TaskGetCurrent(void);
uint8_t MiniOS_TaskDelete(MiniOS_Task *task);
MiniOS_Task *MiniOS_TaskCreate(void (*func)(void *), const char *name, uint32_t staskSize, uint8_t argc, ...);
uint8_t MiniOS_TaskSleep(uint32_t ms);
void MiniOS_TaskInit(void);

#endif /* __MINI_OS_TASK_H */
