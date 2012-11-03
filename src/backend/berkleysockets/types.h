#ifndef BERKLEYSOCKETS_TYPES_H
#define BERKLEYSOCKETS_TYPES_H

#include <stdint.h>
#include <netinet/in.h>

#include "../../types.h"
#include "backend/berkleysockets/constants.h"
#include "datastructure/multireaderringbuffer.h"

typedef struct _GScale_Backend_BerkleySockets_BroadcastPacket
        GScale_Backend_BerkleySockets_BroadcastPacket;

/* the broadcast packet shall be 512 byte at max
 * the reason is that it fits most of the information pretty well
 * and it also is the internet's ipv4 MTU (see RFC 791 page 13)
 */
struct _GScale_Backend_BerkleySockets_BroadcastPacket {
    GScale_Host_UUID srchostuuid;
    GScale_GroupIdentifier gid;

    uint64_t t_sec;
    uint64_t t_usec;
    uint32_t nodecount;

    struct _bpflags{
        int reserved :16;
    } flags;
    uint8_t majorversion;
    uint8_t minorversion;
    uint16_t srcport;
};

typedef struct _GScale_Backend_BerkleySockets_MessageHeader
        GScale_Backend_BerkleySockets_MessageHeader;
typedef struct _GScale_Backend_BerkleySockets_Message
        GScale_Backend_BerkleySockets_Message;

struct _GScale_Backend_BerkleySockets_MessageHeader {
    uint8_t majorversion;
    uint8_t minorversion;

    uint64_t uptimesec;
    uint64_t uptimeusec;

    struct _type {
        int nodeavail :1; /* node is available message */
        int nodeunavail :1; /* node has gone message */
        int data :1; /* plain data message */
        int reserved :5;
    } type;

    uint32_t length;
};
struct _GScale_Backend_BerkleySockets_Message {
    GScale_Backend_BerkleySockets_MessageHeader header;

    union _payload{
        GScale_Packet_Header hdr;
        GScale_Node_FQIdentifier id;
    } payload;

    /*char data[];*/
};

typedef struct _GScale_Backend_BerkleySockets_Socket GScale_Backend_BerkleySockets_Socket;
struct _GScale_Backend_BerkleySockets_Socket{
    int fd;
    int type;
    int family;

    struct _sflags{
        int enabled:1;
        int server:1;
        int msgrcvd:1;  /* 0 if no data has been received yet, 1 if something has been received */
        int reserved:5;
    } flags;

    uint8_t state;
    uint16_t port;

    uint32_t so_sndbps; /* socket send bytes per second */
    uint32_t so_rcvbps; /* socket receive bytes per second */

    GScale_Host_UUID hostuuid; /* the remote host uuid */

    struct timeval created;
    struct timeval synctime;

    uint16_t msgoffset;
    GScale_Backend_BerkleySockets_Message currentmsg;

    mrRingBufferReader *sndreader;
    mrRingBufferReader *eventreader;

    struct _rbuf{
        ringBuffer internalsendbuffer; /* used for messages for this socket only */
        ringBufferMeta internalsendmetabuffer;
        char internalsendcbuffer[2096];  /* space for around 10 avail-messages */
    } ringbuffer;
};

typedef struct _GScale_Backend_BerkleySockets_Class GScale_Backend_BerkleySockets_Class;

struct _GScale_Backend_BerkleySockets_Class {
    int sock_buf_size;

    GScale_Backend_BerkleySockets_Socket *v4mcast;
    GScale_Backend_BerkleySockets_Socket *v6mcast;
    int v4_dgram_socket;
    int v6_dgram_socket;

    ringBufferMeta sndmeta;
    ringBuffer *sndbuffer;
    mrRingBufferMeta sndmrmeta;

    ringBufferMeta eventmeta;
    ringBuffer *eventbuffer;
    mrRingBufferMeta eventmrmeta;

    int evserversockets;
    int evsockets;

    struct timeval so_sndtimeo,so_rcvtimeo;
    uint32_t so_sndbuf,so_rcvbuf;

    uint64_t lastbroadcast;
    int pipefds[2];
};


typedef struct _GScale_Backend_Berkleysockets_Options_CmdPort GScale_Backend_Berkleysockets_Options_CmdPort;

struct _GScale_Backend_Berkleysockets_Options_CmdPort{
    uint64_t id;

    struct _berkopts_flags{
        int add:1;
        int del:1;
        int listen_stream:1;
        int listen_dgram:1;
        int broadcast:1;
/*        int reserverd:27;*/
    }flags;

    struct sockaddr_storage addr;

    GScale_FutureResult fr;
};


#endif
