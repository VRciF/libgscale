/*
 * localsharedmemory_util.c
 *
 *  Created on: Oct 27, 2010
 */

#ifndef LOCALSHAREDMEMORY_C_
#define LOCALSHAREDMEMORY_C_

#define _GNU_SOURCE
#include <errno.h>
#include <stddef.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sysinfo.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "types.h"
#include "crc32/crc32.h"
#include "util/log.h"

#include "datastructure/ringbuffer.h"
#include "backend/internal.h"
#include "backend/sharedmemory.h"
#include "backend/sharedmemory/util.h"
#include "backend/sharedmemory/types.h"

void
GScale_Backend_LocalSharedMemory_Stat(GScale_Backend *_this, struct shmid_ds *shmds)
{
    GScale_Backend_LocalSharedMemory_Storage *storage = NULL;

    if (_this == NULL || _this->backend_private == NULL || shmds == NULL) ThrowInvalidArgumentException();

    storage = (GScale_Backend_LocalSharedMemory_Storage*) _this->backend_private;
    ThrowErrnoOn(shmctl(storage->shmid, IPC_STAT, shmds) == -1);
}

void
GScale_Backend_LocalSharedMemory_FreeBackend(GScale_Backend *_this)
{
    GScale_Backend_LocalSharedMemory_Storage *storage = NULL;
    struct shmid_ds shmds;
    uint8_t locks = 0;

    if (_this == NULL) ThrowInvalidArgumentException();

    storage = (GScale_Backend_LocalSharedMemory_Storage*) _this->backend_private;
    if (storage != NULL) {
        shmds.shm_nattch = 0;
        if (storage->shmid != -1) {
            GScale_Backend_LocalSharedMemory_Stat(_this, &shmds);
        }

        if (storage->shmid != -1 && shmds.shm_nattch == 1) {
            shmctl(storage->shmid, IPC_RMID, NULL);
            storage->shmid = -1;
        }
        if (storage->shm != NULL) {
            shmdt(storage->shm);
            storage->shm = NULL;
        }

        if (storage->semid != -1) {
            /* this also includes decreasing of participant count */
            for (locks = 0; locks < GSCALE_BACK_LSHM_SEM_MAX; locks++) {
                GScale_Backend_LocalSharedMemory_ReleaseLock(storage, locks,
                        IPC_NOWAIT);
            }

            storage->semid = -1;
        }
    }

    _this->ioc.OnLocalNodeAvailable
            = &GScale_Backend_LocalSharedMemory_OnLocalNodeAvailable;
    _this->ioc.OnLocalNodeUnavailable
            = &GScale_Backend_LocalSharedMemory_OnLocalNodeUnavailable;

    _this->ioc.OnLocalNodeWritesToGroup
            = &GScale_Backend_LocalSharedMemory_OnLocalNodeWritesToGroup;

    _this->instance.Worker = NULL;
    _this->instance.GetOption = NULL;
    _this->instance.SetOption = NULL;
    _this->instance.Shutdown = NULL;

    free(_this->backend_private);
    _this->backend_private = NULL;
}

void
GScale_Backend_LocalSharedMemory_IncreaseParticipantCount(GScale_Backend_LocalSharedMemory_Storage *storage)
{
    if (storage == NULL) ThrowInvalidArgumentException();

    while (1) {
        struct sembuf sops[1];
        sops[0].sem_num = GSCALE_BACK_LSHM_SEM_PARTICIPANTCNT;
        sops[0].sem_op = 1;
        sops[0].sem_flg = SEM_UNDO;
        if (semop(storage->semid, sops, 1) == -1) {
            if (errno == EINTR) {
                continue;
            }
            ThrowException("increasing participant count semaphore failed", errno);
        }
        return;
    }
}
void
GScale_Backend_LocalSharedMemory_DecreaseParticipantCount(GScale_Backend_LocalSharedMemory_Storage *storage)
{
    if (storage == NULL) ThrowInvalidArgumentException();

    while (1) {
        struct sembuf sops[1];
        sops[0].sem_num = GSCALE_BACK_LSHM_SEM_PARTICIPANTCNT;
        sops[0].sem_op = -1;
        sops[0].sem_flg = 0;
        if (semop(storage->semid, sops, 1) == -1) {
            if (errno == EINTR) {
                continue;
            }
            ThrowException("decreasing participant count semaphore failed", errno);
        }
        return;
    }
}
int32_t
GScale_Backend_LocalSharedMemory_GetParticipantCount(GScale_Backend_LocalSharedMemory_Storage *storage)
{
    if (storage == NULL) ThrowInvalidArgumentException();

    return semctl(storage->semid, GSCALE_BACK_LSHM_SEM_PARTICIPANTCNT, GETVAL, 0);
}

void
GScale_Backend_LocalSharedMemory_AcquireLock(GScale_Backend_LocalSharedMemory_Storage *storage, uint16_t which,
                                             uint16_t flags, struct timeval *timeout)
{
    struct timespec tspec;
    struct sembuf sops[2];

    if (storage == NULL || which >= GSCALE_BACK_LSHM_SEM_MAX) ThrowInvalidArgumentException();

    if ((storage->locks.lval[which]) != 0) {
        ThrowException("sempahore already locked", EINVAL);
    }

    if(timeout!=NULL){
        tspec.tv_sec  = timeout->tv_sec;
        tspec.tv_nsec = timeout->tv_usec*1000;
    }
    while (1) {
        sops[0].sem_num = which;
        sops[0].sem_op  = 0;
        sops[0].sem_flg = 0; /* this code and the shutdown code shall be the only part which blocks the process */
        sops[1].sem_num = which;
        sops[1].sem_op  = 1;
        sops[1].sem_flg = flags | SEM_UNDO;
        if (semtimedop(storage->semid, sops, 2, timeout==NULL ? NULL : &tspec) == -1) {
            if (errno == EINTR) {
                continue;
            }
            ThrowException("acquire semaphore lock failed", errno);
        }

        break;
    }

    storage->locks.lval[which] = 1;
}
void
GScale_Backend_LocalSharedMemory_ReleaseLock(GScale_Backend_LocalSharedMemory_Storage *storage,
                                             uint16_t which, uint16_t flags)
{
    struct sembuf sops[1];

    if (storage == NULL || which >= GSCALE_BACK_LSHM_SEM_MAX) ThrowInvalidArgumentException();
    if ((storage->locks.lval[which]) == 0) {
        ThrowException("lock not set", EINVAL);
    }

    /* looks like an endless loop, but
     * in case of EINTR i retry to release the lock
     * the problem is, if this doesnt work (which would result in an endless loop here)
     * a lock would be left which is invalid, thus i accept a possible endless loop - which in turn
     * would be very very unnatural, just to be in a consistent state with the locks
     */
    while (1) {
        sops[0].sem_num = which;
        sops[0].sem_op = -1;
        sops[0].sem_flg = flags;

        if (semop(storage->semid, sops, 1) == -1) {
            if (errno == EINTR) {
                continue;
            }
            ThrowException("release semaphore lock failed", errno);
        }
        break;
    }

    storage->locks.lval[which] = 0;
}

uint32_t GScale_Backend_LocalSharedMemory_GetSharedMemorySegmentSize(){
    struct sysinfo info;
    struct shminfo shminfo;
    /* minimum is 3 megabytes */
    uint32_t size = 3 * 1024 * 1024;

    if(sysinfo(&info)<=0){
        return size;
    }
    if ((shmctl(0, IPC_INFO, (struct shmid_ds *) (void *) &shminfo)) < 0){
        return size;
    }

    /* info.freeram, info.mem_unit */
    if((info.totalram*info.mem_unit * 0.01)>shminfo.shmmax){
        size = shminfo.shmmax;
    }
    else{
        size = (uint32_t)(info.totalram*info.mem_unit * 0.01);
    }
    /* 3 megabyte minimum */
    if(size<=3 * 1024 * 1024){
        size = 3 * 1024 * 1024;
    }

    return size;
}

/* multicasting code is from
 * http://www.tenouk.com/Module41c.html
 * many thanks for the example
 */
void GScale_Backend_LocalSharedMemory_Util_CreateMulticastServer(GScale_Backend_LocalSharedMemory_Storage *storage)
{
    char loopch = 0;
    struct in6_addr localInterface;

    if (storage == NULL) ThrowInvalidArgumentException();

    /* Create a datagram socket on which to send. */
    ThrowErrnoOn((storage->multicastserver = socket(AF_INET6, SOCK_DGRAM, 0))<0);

    setsockopt(storage->multicastserver, IPPROTO_IPV6, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch));

    /* Set local interface for outbound multicast datagrams. */
    /* The IP address specified must be associated with a local, */
    /* multicast capable interface. */

    inet_pton(AF_INET6, "::1", &localInterface.s6_addr);
    /*localInterface.s_addr = inet_addr("203.106.93.94");*/
    if(setsockopt(storage->multicastserver, IPPROTO_IPV6, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0)
    {
        close(storage->multicastserver);
        ThrowException("setting interface for server socket failed", errno);
    }

    /* Initialize the group sockaddr structure with a */
    /* group address of 225.1.1.1 and port 5555. */
    memset((char *) &storage->groupSock, 0, sizeof(storage->groupSock));
    ((struct sockaddr_in6*)&storage->groupSock)->sin6_family = AF_INET6;
    inet_pton(AF_INET6, "ff01::1", &((struct sockaddr_in6*)&storage->groupSock)->sin6_addr.s6_addr);
    /*((struct sockaddr_in6*)&storage->groupSock)->sin6_addr.s_addr = inet_addr("226.1.1.1");*/
    ((struct sockaddr_in6*)&storage->groupSock)->sin6_port = htons(4321);
}
void GScale_Backend_LocalSharedMemory_Util_CreateMulticastClient(GScale_Backend_LocalSharedMemory_Storage *storage)
{
    int reuse = 1;
    struct sockaddr_storage localSock;
    struct ipv6_mreq group;

    if (storage == NULL) ThrowInvalidArgumentException();


    /* Create a datagram socket on which to receive. */

    ThrowErrnoOn((storage->multicastclient = socket(AF_INET6, SOCK_DGRAM, 0))<0);

    /* Enable SO_REUSEADDR to allow multiple instances of this */
    /* application to receive copies of the multicast datagrams. */
    setsockopt(storage->multicastclient, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
    fcntl(storage->multicastclient, F_SETFL, fcntl(storage->multicastclient, F_GETFL) | O_NONBLOCK);

    /* Bind to the proper port number with the IP address */
    /* specified as INADDR_ANY. */
    memset((char *) &localSock, 0, sizeof(localSock));
    ((struct sockaddr_in6*)&localSock)->sin6_family = AF_INET6;
    ((struct sockaddr_in6*)&localSock)->sin6_port = htons(4321);
    ((struct sockaddr_in6*)&localSock)->sin6_addr = in6addr_any;
    if(bind(storage->multicastclient, (struct sockaddr*)&localSock, sizeof(localSock)))
    {
        close(storage->multicastclient);
        ThrowException("binding client socket failed", errno);
    }

    /* Join the multicast group 226.1.1.1 on the local 203.106.93.94 */
    /* interface. Note that this IP_ADD_MEMBERSHIP option must be */
    /* called for each local interface over which the multicast */
    /* datagrams are to be received. */
    /*ff02::1:ff00:1234*/
    /*group.ipv6mr_multiaddr.s6_addr = inet_addr("226.1.1.1");*/
    inet_pton(AF_INET6, "ff01::1", &group.ipv6mr_multiaddr.s6_addr);
    group.ipv6mr_interface = 0;
    if(setsockopt(storage->multicastclient, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
    {
        close(storage->multicastclient);
        ThrowException("joining multicast group failed", errno);
    }
}
void GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(GScale_Backend_LocalSharedMemory_Storage *storage)
{
    if (storage == NULL) ThrowInvalidArgumentException();

    sendto(storage->multicastserver, "1", 1, 0, (struct sockaddr*)&storage->groupSock, sizeof(storage->groupSock));
}

#endif
