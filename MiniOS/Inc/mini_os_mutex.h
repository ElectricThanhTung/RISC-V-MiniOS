
#ifndef __MINI_OS_MUTEX_H
#define __MINI_OS_MUTEX_H

#include <stdint.h>

typedef struct {
    volatile const uint32_t Id;
    volatile const uint32_t Nesting;
} MiniOS_Mutex;

void MiniOS_MutexInit(MiniOS_Mutex *mutex);
uint8_t MiniOS_MutexWait(MiniOS_Mutex *mutex, uint32_t timeOut);
uint8_t MiniOS_MutexRelease(MiniOS_Mutex *mutex);

#endif /* __MINI_OS_MUTEX_H */
