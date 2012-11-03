/*
 * localsharedmemory_util.h
 *
 */

#ifndef LOCALSHAREDMEMORY_UTIL_H_
#define LOCALSHAREDMEMORY_UTIL_H_

#include "backend/sharedmemory/types.h"

void GScale_Backend_LocalSharedMemory_AcquireLock(
        GScale_Backend_LocalSharedMemory_Storage *storage, uint16_t which,
        uint16_t flags, struct timeval *timeout);
void GScale_Backend_LocalSharedMemory_ReleaseLock(
        GScale_Backend_LocalSharedMemory_Storage *storage, uint16_t which,
        uint16_t flags);
void GScale_Backend_LocalSharedMemory_IncreaseParticipantCount(
        GScale_Backend_LocalSharedMemory_Storage *storage);
void GScale_Backend_LocalSharedMemory_DecreaseParticipantCount(
        GScale_Backend_LocalSharedMemory_Storage *storage);
int32_t GScale_Backend_LocalSharedMemory_GetParticipantCount(
        GScale_Backend_LocalSharedMemory_Storage *storage);
void GScale_Backend_LocalSharedMemory_Stat(GScale_Backend *_this, struct shmid_ds *shmds);
void GScale_Backend_LocalSharedMemory_FreeBackend(GScale_Backend *_this);

uint32_t GScale_Backend_LocalSharedMemory_GetSharedMemorySegmentSize();

void GScale_Backend_LocalSharedMemory_Util_CreateMulticastServer(GScale_Backend_LocalSharedMemory_Storage *storage);
void GScale_Backend_LocalSharedMemory_Util_CreateMulticastClient(GScale_Backend_LocalSharedMemory_Storage *storage);
void GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(GScale_Backend_LocalSharedMemory_Storage *storage);

#endif
