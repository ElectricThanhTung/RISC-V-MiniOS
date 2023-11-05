
#include <stdlib.h>
#include <stdarg.h>
#include "ch32v00x.h"
#include "mini_os_task.h"

typedef struct {
    void *Next;
    const char *Name;
    uint32_t * volatile StackPointer;
    volatile uint32_t StartTime;
    volatile uint32_t SleepTime;
} MiniOS_SysTask;

static MiniOS_SysTask idleTask;
static MiniOS_SysTask * volatile currentTask;
static MiniOS_SysTask * volatile nextTask;
static MiniOS_SysTask * volatile activeList;
static MiniOS_SysTask * volatile suspendedList;

static volatile uint8_t suspendTaskSwitch;
static uint32_t criticalNesting;

void MiniOS_SetSP(uint32_t *value);

static void MiniOS_SysTickInit(void) {
    SysTick->CMP = SYSTICK_CYCLE;
    SysTick->CNT = 0;
    SysTick->CTLR |= (1 << 0) | (1 << 1);
    NVIC_EnableIRQ(SysTicK_IRQn);
    NVIC_EnableIRQ(Software_IRQn);
    NVIC_SetPriority(Software_IRQn, 0);
}

static void MiniOS_AddToList(MiniOS_SysTask **list, MiniOS_SysTask *node) {
    node->Next = *list;
    *list = node;
}

static uint8_t MiniOS_RemoveFromList(MiniOS_SysTask **list, MiniOS_SysTask *node) {
    MiniOS_SysTask *prv = 0;
    MiniOS_SysTask *n = *list;
    for(; n; n = n->Next) {
        if(n == node) {
            if(prv)
                prv->Next = n->Next;
            else
                *list = n->Next;
            return 1;
        }
        prv = n;
    }
    return 0;
}

static void MiniOS_EnterCritical(void) {
    __disable_irq();
    criticalNesting++;
}

static void MiniOS_ExitCritical(void) {
    if(criticalNesting) {
        if(--criticalNesting == 0)
            __enable_irq();
        return;
    }
    __enable_irq();
}

uint32_t MiniOS_GetTickMs(void) {
    return SysTick->CNT / 6 / 1000;
}

uint32_t MiniOS_GetTickUs(void) {
    return SysTick->CNT / 6;
}

void MiniOS_TaskSwitch(void) {
    if(!suspendTaskSwitch)
        NVIC_SetPendingIRQ(Software_IRQn);
}

void MiniOS_TaskSuspendAll(void) {
    suspendTaskSwitch = 1;
}

void MiniOS_TaskResumeAll(void) {
    suspendTaskSwitch = 0;
}

void MiniOS_SysTickUserHandler(void) {

}

void MiniOS_TaskIdle(void) {
    __WFI();
}

__interrupt __optimize_ofast void SysTick_Handler(void) {
    uint8_t idle = 1;
    MiniOS_SysTask *t = suspendedList;
    MiniOS_SysTask *prv = 0;
    uint32_t tick = SysTick->CNT;
    SysTick->CMP = tick + SYSTICK_CYCLE;
    SysTick->SR = 0x00;

    tick = tick / 6 / 1000;
    while(t) {
        MiniOS_SysTask *next = t->Next;
        if((uint32_t)(tick - t->StartTime) >= t->SleepTime) {
            t->SleepTime = 0;
            if(prv)
                prv->Next = next;
            else
                suspendedList = next;
            MiniOS_AddToList((MiniOS_SysTask **)&activeList, t);
            idle = 0;
        }
        else
            prv = t;
        t = next;
    }

    MiniOS_SysTickUserHandler();
    if(!idle && (nextTask == &idleTask))
        nextTask = activeList;
    MiniOS_TaskSwitch();
}

static void MiniOS_TaskIdleInit(void) {
    static uint32_t idleStack[IDLE_STACK_SIZE + !!(IDLE_STACK_SIZE % 4) * 4];
    MiniOS_EnterCritical();
    idleTask.Next = 0;
    idleTask.Name = "IDLE TASK";

    uint32_t *stackPointer = &idleStack[sizeof(idleStack) / sizeof(idleStack[0]) - 16 - 1];

    stackPointer[0] = (uint32_t)MiniOS_TaskIdle;    /* mepc */
    stackPointer[15] = (uint32_t)MiniOS_TaskIdle;   /* ra */

    idleTask.StackPointer = stackPointer;
    MiniOS_ExitCritical();
}

__optimize_ofast uint32_t *MiniOS_NextStack(uint32_t *stackValue) {
    if(currentTask)
        currentTask->StackPointer = stackValue;
    currentTask = nextTask;
    nextTask = nextTask->Next ? nextTask->Next : (activeList ? activeList : &idleTask);
    return (uint32_t *)currentTask->StackPointer;
}

MiniOS_Task *MiniOS_TaskGetCurrent(void) {
    return (MiniOS_Task *)currentTask;
}

uint8_t MiniOS_TaskDelete(MiniOS_Task *task) {
    MiniOS_SysTask *t = (MiniOS_SysTask *)task;
    if(t == &idleTask)
        return 0;
    MiniOS_EnterCritical();
    if(!MiniOS_RemoveFromList((MiniOS_SysTask **)&activeList, t)) {
        if(!MiniOS_RemoveFromList((MiniOS_SysTask **)&suspendedList, t)) {
            MiniOS_ExitCritical();
            return 0;
        }
    }
    if(currentTask == t) {
        static uint32_t garbageStack[32];
        MiniOS_SetSP(&garbageStack[sizeof(garbageStack) / sizeof(garbageStack[0]) - 1]);
        if(currentTask == nextTask)
            nextTask = nextTask->Next ? nextTask->Next : (activeList ? activeList : &idleTask);
        currentTask = 0;
        free(t);
        MiniOS_ExitCritical();
        while(1)
            MiniOS_TaskSwitch();
    }
    free(t);
    MiniOS_ExitCritical();
    return 1;
}

static void MiniOS_TaskDeleteCurrent(void) {
    MiniOS_TaskDelete((MiniOS_Task *)currentTask);
}

MiniOS_Task *MiniOS_TaskCreate(void (*func)(void *), const char *name, uint32_t staskSize, uint8_t argc, ...) {
    staskSize += sizeof(MiniOS_SysTask);
    staskSize += !!(staskSize % 4) * 4;
    void *mem = malloc(staskSize);
    if(mem) {
        uint8_t i = 0;
        MiniOS_SysTask *taskNode = mem;
        uint32_t *stackPointer = &((uint32_t *)mem)[staskSize / sizeof(uint32_t) - 16 - 1];
        va_list arg_ptr;
        va_start(arg_ptr, argc);
        if(argc > 6)
            argc = 6;

        stackPointer[0] = (uint32_t)func;                       /* mepc */
        stackPointer[1] = 0;                                    /* s1 */
        stackPointer[2] = 0;                                    /* fp */
        stackPointer[3] = 0;                                    /* tp */
        for(; i < argc; i++)
            stackPointer[11 - i] = va_arg(arg_ptr, uint32_t);   /* a(i) */
        va_end(arg_ptr);
        for(; i < 6; i++)
            stackPointer[11 - i] = 0;                           /* a(i) */
        stackPointer[12] = 0;                                   /* t2 */
        stackPointer[13] = 0;                                   /* t1 */
        stackPointer[14] = 0;                                   /* t0 */
        stackPointer[15] = (uint32_t)MiniOS_TaskDeleteCurrent;  /* ra */

        taskNode->Name = name;
        taskNode->StackPointer = stackPointer;
        taskNode->SleepTime = 0;

        MiniOS_EnterCritical();
        MiniOS_AddToList((MiniOS_SysTask **)&activeList, taskNode);
        MiniOS_ExitCritical();
        return (MiniOS_Task *)taskNode;
    }
    return 0;
}

uint8_t MiniOS_TaskSleep(uint32_t ms) {
    if(currentTask == &idleTask)
        return 0;
    if(ms) {
        MiniOS_EnterCritical();
        currentTask->StartTime = MiniOS_GetTickMs();
        currentTask->SleepTime = ms;
        MiniOS_RemoveFromList((MiniOS_SysTask **)&activeList, currentTask);
        MiniOS_AddToList((MiniOS_SysTask **)&suspendedList, currentTask);
        if(currentTask == nextTask)
            nextTask = nextTask->Next ? nextTask->Next : (activeList ? activeList : &idleTask);
        MiniOS_ExitCritical();
        while(currentTask->SleepTime)
            MiniOS_TaskSwitch();
    }
    return 1;
}

void MiniOS_TaskInit(void) {
    static MiniOS_SysTask defaultTask;
    criticalNesting = 0;
    MiniOS_EnterCritical();
    MiniOS_SysTickInit();
    activeList = &defaultTask;
    activeList->Next = 0;
    activeList->SleepTime = 0;
    currentTask = activeList;
    nextTask = activeList;
    suspendedList = 0;
    suspendTaskSwitch = 0;
    MiniOS_TaskIdleInit();
    MiniOS_ExitCritical();
}
