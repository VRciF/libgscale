/*
 * localsharedmemory.h
 *
 *  Created on: Oct 27, 2010
 */

#ifndef LOCALSHAREDMEMORY_H_
#define LOCALSHAREDMEMORY_H_

/*
 * this backend is used if you want to communicate with other processes on localhost
 * how does it work?
 * there are 2 parts
 * firstly im using semaphores to synchronize
 * secondly the shared memory which is used for communication and maintenance
 *
 * the workflow is like this:
 * a bunch of semaphores is allocated for the following:
 *     a) writing a simple message in shared memory
 *     b) update the count of readers of current message
 *            to understand this, think about the question: how do i know if a filled buffer has been read by all participants?
 *            so i need a counter which tells me if every shared memory user (a) has read the buffer (b)
 *     c) maintenance lock
 *            since the shared memory is used across all processes, there is no process which is some kind of server
 *            thus if a process has got time to do maintenance, it can lock the memory and clean up stuff
 *            this also means, the last process (b drops to zero) removes the shared memory and semaphores
 *
 * the idea is that each semaphore corresponds to a specific area of the shared memory
 * thus it will be possible to connect to the shared mem e.g. by incrementing users count
 * but without interrupting writes to the message buffer
 */

#include <stdint.h>
#include <sys/time.h>

#include "backend/sharedmemory/types.h"
#include "types.h"

/* called when a local node becomes available */
void GScale_Backend_LocalSharedMemory_OnLocalNodeAvailable(
        GScale_Backend *_this, GScale_Node *node);
/* called when a local node becomes unavailable */
void GScale_Backend_LocalSharedMemory_OnLocalNodeUnavailable(GScale_Backend *_this,
        GScale_Node *node);
/* called when a local node writes data to the group */
int32_t GScale_Backend_LocalSharedMemory_OnLocalNodeWritesToGroup(GScale_Backend *_this, GScale_Packet *packet);

void GScale_Backend_LocalSharedMemory_Worker(GScale_Backend *_this, struct timeval *timeout);
void GScale_Backend_LocalSharedMemory_Shutdown(GScale_Backend *_this);
/* backend init function */
void GScale_Backend_LocalSharedMemory_Init(GScale_Backend *_this);

#define GScale_Backend_LocalSharedMemory &GScale_Backend_LocalSharedMemory_Init

#endif /* LOCALSHAREDMEMORY_H_ */
