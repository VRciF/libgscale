/*
 * types.h
 *
 *  Created on: Oct 27, 2010
 */

#ifndef LOCALSHAREDMEMORY_TYPES_H_
#define LOCALSHAREDMEMORY_TYPES_H_

#include <stdint.h>
#include <sys/sem.h>
#include <netinet/in.h>

#include "datastructure/ringbuffer.h"
#include "../../types.h"

#ifdef _SEM_SEMUN_UNDEFINED
union semun {
    int val; /*<= value for SETVAL*/
    struct semid_ds *buf; /*<= buffer for IPC_STAT & IPC_SET*/
    unsigned short int *array; /*<= array for GETALL & SETALL*/
    struct seminfo *__buf; /*<= buffer for IPC_INFO*/
};
#endif

enum GSCALE_BACKEND_LOCALSHAREDMEMORY_SEMAPHORES {
    GSCALE_BACK_LSHM_SEM_WRITELOCK=0, GSCALE_BACK_LSHM_SEM_MSGREADLOCK,
    GSCALE_BACK_LSHM_SEM_MAINTENANCELOCK,

    GSCALE_BACK_LSHM_SEM_PARTICIPANTCNT, GSCALE_BACK_LSHM_SEM_MAX
};

typedef struct _GScale_Backend_LocalSharedMemory_Storage
        GScale_Backend_LocalSharedMemory_Storage;
typedef struct _GScale_Backend_LocalSharedMemory_Buffer
        GScale_Backend_LocalSharedMemory_Buffer;
typedef struct _GScale_Backend_LocalSharedMemory_Message
        GScale_Backend_LocalSharedMemory_Message;
typedef struct _GScale_Backend_LocalSharedMemory_MessageHeader
        GScale_Backend_LocalSharedMemory_MessageHeader;

struct _GScale_Backend_LocalSharedMemory_Buffer {
    ringBufferMeta meta;
    char cshm[1]; /* the length is not limited to 1, but this is done to prevent compiler warnings */
};

struct _GScale_Backend_LocalSharedMemory_Storage {
    int multicastserver;
    struct sockaddr_storage groupSock;

    int multicastclient;

    union _locks {
        struct _lock {
            int write;
            int read;
            int maintenance;
        } lock;
        int lval[3];
    } locks;

    struct _flags{
        int particpantcounted;
    } flags;
    /* counts the messages written in the buffer
     * this is used to identify every message and thus
     */
    int32_t semid;
    int32_t shmid;

    /* this equals the buffer within the shared memory, except the offsets
     * correspond to those i have already read
     */
    ringBufferMeta readbuffer;

    /* data pointer goes to shm->cshm
     * meta pointer to shm->meta
     */
    ringBuffer sharedbuffer;
    GScale_Backend_LocalSharedMemory_Buffer *shm;
};

/* this is the structure for messages which are stored within the ringbuffer/shared memory */
struct _GScale_Backend_LocalSharedMemory_MessageHeader {
    uint8_t majorversion;
    uint8_t minorversion;
    int32_t readcnt; /* how many participants have read this message */

    struct _type {
        int nodeavail :1; /* node is available message */
        int nodeunavail :1; /* node has gone message */
        int data :1; /* plain data message */
        int reserved :5;
    } type;

    uint32_t length; /* size of message */
};
struct _GScale_Backend_LocalSharedMemory_Message {
    GScale_Backend_LocalSharedMemory_MessageHeader header;

    union _payload{
        GScale_Packet_Header hdr;
        GScale_Node_FQIdentifier id;
    } payload;

    /*char data[];*/
};

#endif /* LOCALSHAREDMEMORY_H_ */
