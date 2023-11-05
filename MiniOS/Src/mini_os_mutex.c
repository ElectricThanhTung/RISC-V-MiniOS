
#include "mini_os_task.h"
#include "mini_os_mutex.h"

void MiniOS_MutexInit(MiniOS_Mutex *mutex) {
    MiniOS_TaskSuspendAll();
    *(uint32_t *)&mutex->Id = 0;
    *(uint32_t *)&mutex->Nesting = 0;
    MiniOS_TaskResumeAll();
}

uint8_t MiniOS_MutexWait(MiniOS_Mutex *mutex, uint32_t timeOut) {
    uint32_t startTick = MiniOS_GetTickMs();
    uint32_t currentId = (uint32_t)MiniOS_TaskGetCurrent();

    MiniOS_TaskSuspendAll();
    if(mutex->Id == 0) {
        *(uint32_t *)&mutex->Id = currentId;
        *(uint32_t *)&mutex->Nesting = 1;
        MiniOS_TaskResumeAll();
        return 1;
    }
    else if(mutex->Id == currentId) {
        if(mutex->Nesting < 0xFFFFFFFF) {
            (*(uint32_t *)&mutex->Nesting)++;
            MiniOS_TaskResumeAll();
            return 1;
        }
        MiniOS_TaskResumeAll();
        return 0;
    }

    MiniOS_TaskResumeAll();
    while(1) {
        MiniOS_TaskSuspendAll();
        if(!mutex->Id) {
            MiniOS_TaskResumeAll();
            return 1;
        }
        else if((uint32_t)(MiniOS_GetTickMs() - startTick) >= timeOut) {
            MiniOS_TaskResumeAll();
            return 0;
        }
        MiniOS_TaskResumeAll();
        MiniOS_TaskSwitch();
    }
    return 0;
}

uint8_t MiniOS_MutexRelease(MiniOS_Mutex *mutex) {
    uint32_t currentId = (uint32_t)MiniOS_TaskGetCurrent();
    MiniOS_TaskSuspendAll();
    if(mutex->Id == 0) {
        MiniOS_TaskResumeAll();
        return 1;
    }
    else if(mutex->Id == currentId) {
        if(mutex->Nesting) {
            if(--(*(uint32_t *)&mutex->Nesting) == 0)
                *(uint32_t *)&mutex->Id = 0;
        }
        MiniOS_TaskResumeAll();
        return 1;
    }
    MiniOS_TaskResumeAll();
    return 0;
}
