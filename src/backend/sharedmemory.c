/*
 * localsharedmemory.h
 *
 *  Created on: Oct 27, 2010
 */

#ifndef LOCALSHAREDMEMORY_C_
#define LOCALSHAREDMEMORY_C_

/*
 * this backend is used if you want to communicate with other processes on localhost
 * how does it work?
 * there are 2 parts
 * 1: im using semaphores to synchronize
 * 2: the shared memory which is used for communication and maintenance
 *
 * the workflow is like this:
 * a bunch of semaphores is allocated for the following:
 *     a] writing a simple message in shared memory
 *     b] update the count of readers of current message
 *            to understand this, think about the question: how do i know if a filled buffer has been read by all participants?
 *            so i need a counter which tells me if every shared memory user a] has read the buffer b]
 *     c] maintenance lock
 *            since the shared memory is used across all processes, there is no process which is some kind of server
 *            thus if a process has got time to do maintenance, it can lock the memory and clean up stuff
 *            this also means, the last process ( if b] drops to zero ) removes the shared memory and semaphores
 *
 * the idea is that each semaphore corresponds to a specific area of the shared memory
 * thus it will be possible to connect to the shared mem e.g. by incrementing users count
 * but without interrupting writes to the message buffer
 */

#define _GNU_SOURCE
#include <errno.h>
#include <stddef.h>

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/epoll.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "types.h"
#include "exception.h"
#include "crc32/crc32.h"
#include "util/log.h"

#include "datastructure/ringbuffer.h"
#include "backend/internal.h"
#include "backend/sharedmemory.h"
#include "backend/sharedmemory/util.h"
#include "backend/sharedmemory/types.h"

void
GScale_Backend_LocalSharedMemory_OnLocalNodeAvailable(GScale_Backend *_this, GScale_Node *node)
{
    GScale_Backend_LocalSharedMemory_Message msg;
    GScale_Backend_LocalSharedMemory_Storage *storage = NULL;
    int32_t readcnt = 0;
    struct timeval timeout;
    int32_t length = 0;
    GScale_Exception except;

    /* seems localnode has connected
     * so we notify the other nodes
     */
    storage = (GScale_Backend_LocalSharedMemory_Storage*) _this->backend_private;
    if (storage == NULL) {
        ThrowException("no backend initialized", EINVAL);
    }

    if(storage->flags.particpantcounted==0){
        GScale_Backend_LocalSharedMemory_IncreaseParticipantCount(storage);
        storage->flags.particpantcounted = 0x01;
    }

    readcnt = GScale_Backend_LocalSharedMemory_GetParticipantCount(storage);
    readcnt--; /* decrease, so we dont count _this */
    if (readcnt <= 0) {
        return;
    }

    msg.header.readcnt = readcnt;
    msg.header.type.nodeavail |= 1;
    msg.header.type.nodeunavail = 0;
    msg.header.type.data = 0;
    msg.header.length = sizeof(msg.payload.id);
    GScale_Backend_Internal_SetFQNodeIdentifierByNode(node, &msg.payload.id);

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; /* 100 milliseconds */

    GScale_Backend_LocalSharedMemory_AcquireLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, SEM_UNDO, &timeout);

    Try{
        length = sizeof(msg.header)+sizeof(msg.payload.id);
        ringBuffer_writeExt(&storage->sharedbuffer, &msg, length, GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);
    }
    Catch(except){
        GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, 0);
        Throw except;
    }
    GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, 0);

    GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(storage);
}

void
GScale_Backend_LocalSharedMemory_OnLocalNodeUnavailable(GScale_Backend *_this, GScale_Node *node)
{
    GScale_Backend_LocalSharedMemory_Message msg;
    GScale_Backend_LocalSharedMemory_Storage *storage = NULL;
    int32_t readcnt = 0;
    struct timeval timeout;
    int32_t length = 0;
    GScale_Exception except;

    /* seems localnode has connected
     * so we notify the other nodes
     */
    storage = (GScale_Backend_LocalSharedMemory_Storage*) _this->backend_private;
    if (storage == NULL) {
        ThrowException("no backend initialized", EINVAL);
    }

    readcnt = GScale_Backend_LocalSharedMemory_GetParticipantCount(storage);
    readcnt--; /* decrease me */
    if (readcnt <= 0) {
        return;
    }

    msg.header.readcnt = readcnt;
    msg.header.type.nodeavail = 0;
    msg.header.type.nodeunavail |= 1;
    msg.header.type.data = 0;
    msg.header.length = sizeof(msg.payload.id);

    GScale_Backend_Internal_SetFQNodeIdentifierByNode(node, &msg.payload.id);

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; /* 100 milliseconds */

    GScale_Backend_LocalSharedMemory_AcquireLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, SEM_UNDO, &timeout);

    Try{
        length = sizeof(msg.header)+sizeof(msg.payload.id);
        ringBuffer_writeExt(&storage->sharedbuffer, &msg, length, GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);

        if(GScale_Backend_Internal_CountLocalNodes(_this->g)<=1 && storage->flags.particpantcounted==1){
            GScale_Backend_LocalSharedMemory_DecreaseParticipantCount(storage);
            storage->flags.particpantcounted = 0;
        }
    }
    Catch(except){
        GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, 0);
        Throw except;
    }

    GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, 0);

    GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(storage);
}

int32_t
GScale_Backend_LocalSharedMemory_OnLocalNodeWritesToGroup(GScale_Backend *_this, GScale_Packet *packet)
{
    GScale_Backend_LocalSharedMemory_Message msg;
    GScale_Backend_LocalSharedMemory_Storage *storage = NULL;
    uint16_t headersize = 0;
    uint32_t writelength = 0;
    int32_t readcnt = 0;
    void *ptr[4] = { NULL, NULL, NULL, NULL };
    int32_t lengths[4] = { 0, 0, 0, 0 };
    GScale_Exception except;

    storage = (GScale_Backend_LocalSharedMemory_Storage*) _this->backend_private;
    if (storage == NULL) {
        ThrowException("no backend initialized", EINVAL);
    }

    GScale_Backend_LocalSharedMemory_AcquireLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, SEM_UNDO, NULL);

    Try{
        headersize = sizeof(msg.header)+sizeof(packet->header);

        writelength = ringBuffer_getFIONWRITE(&storage->sharedbuffer);
        if (writelength > headersize) {

            readcnt = GScale_Backend_LocalSharedMemory_GetParticipantCount(storage);
            readcnt--; /* decrease me */
            if (readcnt <= 0) {
                return 0;
            }
            writelength -= headersize;
            writelength = writelength > packet->data.length ? packet->data.length : writelength;

            msg.header.readcnt = readcnt;
            msg.header.type.nodeavail = 0;
            msg.header.type.nodeunavail = 0;
            msg.header.type.data |= 1;
            msg.header.length = sizeof(packet->header) + writelength;

            ptr[0] = &msg.header;
            ptr[1] = &packet->header;
            ptr[2] = packet->data.buffer;
            lengths[0] = sizeof(msg.header);
            lengths[1] = sizeof(packet->header);
            lengths[2] = writelength;
            ringBuffer_multiWriteExt(&storage->sharedbuffer, ptr, lengths, GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);

        }
    }
    Catch(except){
        GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, 0);
        Throw except;
    }
    GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_WRITELOCK, 0);

    GScale_Backend_LocalSharedMemory_Util_NotifyMulticastEvent(storage);

    return writelength;
}

void GScale_Backend_LocalSharedMemory_Shutdown(GScale_Backend *_this){
    GScale_Backend_LocalSharedMemory_FreeBackend(_this);
}

void
GScale_Backend_LocalSharedMemory_Init(GScale_Backend *_this)
{
    uint32_t ipckey = 0;
    uint8_t oncemore = 1;
    struct shmid_ds shmds;
    int semit = 0;
    union semun arg;
    struct semid_ds buf;
    int tries = 0, maxtries = 3;
    uint8_t createshm = 1;
    GScale_Backend_LocalSharedMemory_Storage *storage = NULL;
    void *memory = NULL;
    ringBuffer tmpbuffer;
    uint32_t rlength = 0;
    GScale_Exception except;

    if (_this == NULL) ThrowInvalidArgumentException();

    /* allocate memory for synchronization */
    _this->backend_private = malloc(
            sizeof(GScale_Backend_LocalSharedMemory_Storage));
    /* Throw Exception if malloc failed */
    ThrowErrnoOn(_this->backend_private == NULL);

    storage = (GScale_Backend_LocalSharedMemory_Storage*) _this->backend_private;

    storage->locks.lval[0] = 0;
    storage->locks.lval[1] = 0;
    storage->locks.lval[2] = 0;
    storage->semid = -1;
    storage->shmid = -1;
    storage->flags.particpantcounted = 0;

    Try{
        GScale_Backend_LocalSharedMemory_Util_CreateMulticastServer(storage);
        GScale_Backend_LocalSharedMemory_Util_CreateMulticastClient(storage);
        GScale_Backend_Internal_AddEventDescriptor(_this->g, storage->multicastclient, EPOLLIN);

        /* initialize semaphores */
        ipckey = crc32buf(_this->g->gid.nspname,
                sizeof(_this->g->gid.nspname));

        while (oncemore) {
            storage->semid = semget(ipckey, GSCALE_BACK_LSHM_SEM_MAX, 0777
                    | IPC_CREAT | IPC_EXCL);
            if (storage->semid >= 0) {
                /* semaphore is created, so lets initialize it */
                for (semit = 0; semit < GSCALE_BACK_LSHM_SEM_MAX; semit++) {
                    arg.val = 0;
                    semctl(storage->semid, semit, SETVAL, arg);
                }
            } else {
                storage->semid = semget(ipckey, GSCALE_BACK_LSHM_SEM_MAX, 0777);
                if (storage->semid == -1) {
                    ThrowException("creating semaphore failed", errno);
                } else {
                    arg.buf = &buf;
                    for (tries = 0; tries < maxtries; tries++) {
                        if (semctl(storage->semid,
                                GSCALE_BACK_LSHM_SEM_MAINTENANCELOCK, IPC_STAT, arg)
                                == -1) {

                            ThrowException("waiting for maintenance lock failed", errno);
                        }
                        if (arg.buf->sem_otime != 0) {
                            break;
                        } else {
                            usleep(100000);
                        }
                    }
                    if (tries >= maxtries) {
                        ThrowException("waiting for semaphore initialization timed out", errno);
                    }
                }
            }

            /* the semaphore has been created, now we need check if this is an empty semaphore */
            /* 1. lock the init semaphore - this is only done here and by every process who wants access */
            Try{
                GScale_Backend_LocalSharedMemory_AcquireLock(storage, GSCALE_BACK_LSHM_SEM_MAINTENANCELOCK, SEM_UNDO, NULL);
            }
            Catch(except){
                /* well the operation failed, maybe the semaphore has been removed */
                if (except.code == EIDRM || except.code == EINTR) { /* semaphore has been removed */
                    continue; /* once again */
                } else { /* something went really wrong, there is nothing i can do here */
                    Throw except;
                }
            }

            /* locked the semaphore */
            /* now increase the process count */
            /* process count can only be undone by having locked the semaphoreset via maintenance or a process crashed*/
            createshm = 1;

            /* first try to get shared memory segment */
            storage->shmid = shmget(ipckey, 0, 0777);
            if (storage->shmid == -1) {
                if (errno != ENOENT) {
                    /* not much we can do here */
                    ThrowException("allocating shared memory failed", errno);
                }
                /* create the segment newly */
            } else {
                storage->shm = (GScale_Backend_LocalSharedMemory_Buffer*) shmat(
                        storage->shmid, NULL, 0);

                GScale_Backend_LocalSharedMemory_Stat(_this, &shmds);

                if (shmds.shm_nattch == 1) {
                    /* this means im the only user of a segment which already existed - weird
                     * maybe it has been left over cause of a crash altough it shouldnt
                     * so drop the segment
                     */
                    shmctl(storage->shmid, IPC_RMID, NULL);
                    shmdt(storage->shm);
                    storage->shm = NULL;
                    storage->shmid = -1;

                    /* create the segment newly */
                } else {
                    /* just use the segment */
                    createshm = 0;

                    Try{
                        GScale_Backend_LocalSharedMemory_Stat(_this, &shmds);
                    }
                    Catch_anonymous{
                        shmds.shm_segsz = GScale_Backend_LocalSharedMemory_GetSharedMemorySegmentSize();
                    }

                    ringBuffer_initMetaPointer(&storage->sharedbuffer, &storage->shm->meta);
                }
            }

            if (createshm) {

                /* memory size depends on total ram on host
                 * at max it's maxsharedmemory segment size
                 * minimum should be 1 percent of RAM
                 */
                storage->shmid = shmget(ipckey, GScale_Backend_LocalSharedMemory_GetSharedMemorySegmentSize(), 0777 | IPC_CREAT);
                if (storage->shmid == -1) {
                    /* creating the shared memory failed - just drop everything and break */
                    ThrowException("allocating shared memory failed", errno);
                }
                Try{
                    GScale_Backend_LocalSharedMemory_Stat(_this, &shmds);
                }
                Catch_anonymous{
                    shmds.shm_segsz = GScale_Backend_LocalSharedMemory_GetSharedMemorySegmentSize();
                }
                if(shmds.shm_segsz <= sizeof(GScale_Backend_LocalSharedMemory_Buffer)){
                    ThrowException(strerror(ENOMEM), ENOMEM);
                }

                memory = shmat(storage->shmid, NULL, 0);
                /* now drop the segment - this comes only into play after the last process
                 * detaches from the segment, thus it should be always cleaned up automatically
                 * e.g. in case of a crash
                 */
                if (memory == NULL) {
                    /* creating the shared memory failed - just drop everything and break */
                    ThrowException("attaching shared memory failed", errno);
                }
                memset(memory, '\0', shmds.shm_segsz);

                storage->shm = (GScale_Backend_LocalSharedMemory_Buffer*) memory;

                ringBuffer_initMetaPointer(&storage->sharedbuffer, &storage->shm->meta);
                /* initialize ring buffer */
                ringBuffer_reset(&storage->sharedbuffer);

            }

            oncemore = 0;
        }

        ringBuffer_initDataPointer(&storage->sharedbuffer,
                                  shmds.shm_segsz-offsetof(GScale_Backend_LocalSharedMemory_Buffer, cshm),
                                  ((char*)storage->shm)+offsetof(GScale_Backend_LocalSharedMemory_Buffer, cshm));

        /* grab a copy of the current state buffer */
        storage->readbuffer = storage->shm->meta;
        tmpbuffer = storage->sharedbuffer;
        tmpbuffer.meta = &storage->readbuffer;
        /* ignore everything that has not been read until now
         * begin with a completely empty copy of the buffer
         */
        rlength = ringBuffer_getFIONREAD(&tmpbuffer);
        if (rlength > 0) {
            ringBuffer_readExt(&tmpbuffer, NULL, rlength, GSCALE_RINGBUFFER_OPTION_SKIPDATA);
        }
        GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_MAINTENANCELOCK, 0);

    }
    Catch(except){
        GScale_Backend_LocalSharedMemory_FreeBackend(_this);
        Throw except;
    }

    _this->ioc.OnLocalNodeAvailable
            = &GScale_Backend_LocalSharedMemory_OnLocalNodeAvailable;
    _this->ioc.OnLocalNodeUnavailable
            = &GScale_Backend_LocalSharedMemory_OnLocalNodeUnavailable;

    _this->ioc.OnLocalNodeWritesToGroup
            = &GScale_Backend_LocalSharedMemory_OnLocalNodeWritesToGroup;

    _this->instance.Worker = &GScale_Backend_LocalSharedMemory_Worker;
    _this->instance.GetOption = NULL;
    _this->instance.SetOption = NULL;
    _this->instance.Shutdown = &GScale_Backend_LocalSharedMemory_Shutdown;
}

void
GScale_Backend_LocalSharedMemory_Worker(GScale_Backend *_this, struct timeval *timeout)
{
    GScale_Backend_LocalSharedMemory_Storage *storage = NULL;
    ringBufferMeta copym;
    ringBuffer copy;
    ringBufferMemMap memmap;
    GScale_Backend_LocalSharedMemory_Message msg;
    uint8_t skipit = 0;
    GScale_Packet message;
    uint32_t length = 0;
    uint32_t maxread = 0;
    char nullbuffer[1024] = { '\0' };
    GScale_Exception except;

    if (_this == NULL || _this->backend_private == NULL) ThrowInvalidArgumentException();

    storage = (GScale_Backend_LocalSharedMemory_Storage*) _this->backend_private;
    /* grab read lock and read the buffer */

    /* just read out multicastclient */
    while(read(storage->multicastclient, nullbuffer, sizeof(nullbuffer)) > 0){}

    GScale_Backend_LocalSharedMemory_AcquireLock(storage, GSCALE_BACK_LSHM_SEM_MSGREADLOCK, SEM_UNDO | IPC_NOWAIT, timeout);

    Try{
        /* problem(s):
         * o) i need to separatly keep track of the readoffset or so i dont waste cpu on messages already parsed
         *    not sure how i can solve this easily, also the readoffset needs to be changed
         *    by the single last reader of the message
         *
         * solved:
         * o) messages from my own host, need to be reread again and just droped
         *    not sure if there is a more efficient way - this one is the easiest
         * o) the problem was that its not sure which message has been written
         *    e.g. in case the ringbuffer swaps from the end of the buffer to the beginning
         *    because there are 2 memcpy operations necessary or to say it in another way
         *    the cpu copies the memory not by using an atomic operation
         *
         *    the solution was that the writeoffset - and thus the remaining memory
         *    which can be read is changed by the ringBuffer_Write operation in just one line
         *    at the end of the function. this means if the writeoffset is changed
         *    the data will be sure to be readable
         */
        /* read lock acquired, lets have a look */
        /* first we get the message header */

        copym = storage->readbuffer;
        copym.writepos = storage->sharedbuffer.meta->writepos;
        copy = storage->sharedbuffer;
        copy.meta = &copym;

        if (!ringBuffer_isEmpty(&copy) &&
            ringBuffer_initMemMap(&copy, &memmap, &msg.header, sizeof(msg.header)) == GSCALE_OK &&
            ringBuffer_readExt(&copy, (char *)&msg.header, sizeof(msg.header), GSCALE_RINGBUFFER_OPTION_ALLORNOTHING) > 0) {

            if (msg.header.type.nodeavail || msg.header.type.nodeunavail) {
                if (ringBuffer_readExt(&copy, (char *)&msg.payload.id, sizeof(msg.payload.id),
                        GSCALE_RINGBUFFER_OPTION_ALLORNOTHING) > 0) {
                    if (GScale_Backend_Internal_HostUUIDEqual(
                            GScale_Backend_Internal_GetBinaryHostUUID(),
                            (const GScale_Host_UUID*) &msg.payload.id.hostuuid)) {
                        skipit = 1;
                    } else {
                        /* nice a host is available */

                        /* add remote node */
                        if (msg.header.type.nodeavail) {
                            GScale_Backend_Internal_OnNodeAvailable(_this, &msg.payload.id, NULL);
                        } else {
                            GScale_Backend_Internal_OnNodeUnavailable(_this, &msg.payload.id);
                        }
                    }
                }
            } else if (msg.header.type.data) {
                if (ringBuffer_readExt(&copy, (char *)&message.header, sizeof(message.header),
                        GSCALE_RINGBUFFER_OPTION_ALLORNOTHING) > 0) {
                    if (GScale_Backend_Internal_HostUUIDEqual(
                            GScale_Backend_Internal_GetBinaryHostUUID(),
                            (const GScale_Host_UUID*) &message.header.src.hostuuid)) {
                        skipit = 1;
                    } else {
                        message.data.length = msg.header.length - sizeof(GScale_Packet_Header);

                        /* read the rest of the package and forwared as a remote packet */

                        /* the idea is the following
                         * im using ringbuffers dma method to get a pointer of the shared memory
                         * using a const pointer i forward the data to the data to the client code
                         * if the buffer wraps around for this only message, i will use 2 calls to
                         * clients read method
                         */

                        /* 1) grab dma pointer of ringbuffer message */
                        length = message.data.length;

                        while (length > 0) {
                            maxread = (uint32_t)ringBuffer_dmaRead(&copy, (void**)&message.data.buffer);
                            if (maxread <= 0) {
                                break;
                            }

                            message.data.length
                                    = maxread > message.data.length ? message.data.length
                                            : maxread;
                            ringBuffer_readExt(&copy, NULL, message.data.length,
                                    GSCALE_RINGBUFFER_OPTION_SKIPDATA);

                            GScale_Backend_Internal_SendLocalMessage(_this,
                                    &message);

                            length -= message.data.length;
                        }

                    }
                }
            }

            if(!skipit){
                msg.header.readcnt--;
                if (msg.header.readcnt <= 0) {
                    /* this messages isnt needed any more, skip it */
                    ringBuffer_readExt(&storage->sharedbuffer, NULL, msg.header.length + sizeof(msg.header), GSCALE_RINGBUFFER_OPTION_SKIPDATA);
                }
                ringBuffer_writeMemMap(&copy, &memmap,
                        GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);
            }

            storage->readbuffer = copym;
        }
    }
    Catch(except){
        GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_MSGREADLOCK, 0);
        Throw except;
    }

    GScale_Backend_LocalSharedMemory_ReleaseLock(storage, GSCALE_BACK_LSHM_SEM_MSGREADLOCK, 0);
}

#endif /* LOCALSHAREDMEMORY_H_ */
