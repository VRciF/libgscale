/*
 * berkleysockets.h
 *
 *  Created on: Oct 22, 2010
 *
 *  many thanks go to beej's guide to network programming
 *  http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 *
 *  the solution for dynamic buffer size is inspired by M. Tim Jones article
 *  http://www.ibm.com/developerworks/linux/library/l-hisock.html
 *  altough not implemented yet
 *
 *  from http://en.wikipedia.org/wiki/IP_multicast
 *     IPv6 does not implement broadcast addressing and replaces it with multicast to the specially-defined all-nodes multicast address
 *  note:
 *  the case of multicasting is not yet implemented since im not yet sure which aspects shall use multicasting since its unreliable
 */

#ifndef BERKLEYSOCKETS_C_
#define BERKLEYSOCKETS_C_

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/epoll.h>
#include <ifaddrs.h>

#include <stdio.h>

#include "backend/internal.h"
#include "backend/berkleysockets.h"
#include "backend/berkleysockets/types.h"
#include "backend/berkleysockets/socket.h"
#include "backend/berkleysockets/udp.h"
#include "backend/berkleysockets/netspeed.h"
#include "backend/berkleysockets/util.h"
#include "backend/berkleysockets/database.h"
#include "database.h"
#include "exception.h"
#include "util/log.h"
#include "debug.h"

/*********************************** helper functions **********************************************/

void
GScale_Backend_BerkleySockets_handleSockets(GScale_Backend *_this, struct timeval *timeout)
{
    GScale_Backend_BerkleySockets_Class *instance =
            (GScale_Backend_BerkleySockets_Class*) _this->backend_private;

    if(instance == NULL) ThrowInvalidArgumentException();

    GScale_Backend_BerkleySockets_handleEpoll(_this, timeout, instance->evserversockets);
    GScale_Backend_BerkleySockets_handleEpoll(_this, timeout, instance->evsockets);

}

void
GScale_Backend_BerkleySockets_handleEpoll(GScale_Backend *_this, struct timeval *timeout, int evsocket)
{

    struct epoll_event events[128];
    int n=0;
    int nfds = 0;
    int32_t milliseconds = 0;
    /*char ip[INET6_ADDRSTRLEN+1] = {'\0'};*/

    GScale_Backend_BerkleySockets_Class *instance =
            (GScale_Backend_BerkleySockets_Class*) _this->backend_private;

    if(_this==NULL || instance==NULL) ThrowInvalidArgumentException();

    if(timeout!=NULL){
        milliseconds = timeout->tv_sec*1000;
        milliseconds = milliseconds + (int32_t)(timeout->tv_usec/1000);
    }
    nfds = epoll_wait(evsocket, events, 128, milliseconds);
    for(n=0; n < nfds; ++n) {
        char where[128] = {'\0'};
        GScale_Backend_BerkleySockets_Socket *socket = NULL;
        /*GScale_Backend_BerkleySockets_Socket *socket = (GScale_Backend_BerkleySockets_Socket*)events[n].data.ptr;*/
        snprintf(where, sizeof(where)-1, "fd=%d", events[n].data.fd);
        Try{
        socket = GScale_Backend_Berkleysockets_Database_getSocket(_this->g->dbhandle, where, NULL);
        }
        Catch_anonymous{
            continue;
        }

        if(events[n].events & EPOLLOUT){
            GScale_Backend_BerkleySockets_Socket_disableEPOLLOUT(_this, socket);
        }

        if(socket->flags.server){
            struct sockaddr_storage addr;
            socklen_t addrlen = sizeof(addr);

            switch(socket->type){
                case SOCK_DGRAM:
                    {
                        union _dgrampacket{
                            GScale_Backend_BerkleySockets_BroadcastPacket bcast;
                            char buffer[64*1024];  /* 64k packet */
                        } packet;
                        int numbytes = 0;
                        int uuidcmp = 0;
                        GSCALE_DEBUG("datagram received");

                        if ((numbytes = recvfrom(socket->fd, packet.buffer, sizeof(packet.buffer) , 0, (struct sockaddr *)&addr, &addrlen)) > 0) {
                            /* if groupid differs ignore packet */
                            if(GScale_Backend_Internal_CompareGroupIdentifier(&packet.bcast.gid, &_this->g->gid)!=0){
                            	break;
                            }

                            /* check if already connected */
                            if(GScale_Backend_Berkleysockets_Database_getSocketBySettings(_this->g->dbhandle, NULL, -1, -1, -1, 0,
                                                                                          (const GScale_Host_UUID*)&packet.bcast.srchostuuid)!=NULL){
                                /* already connected */
                                break;
                            }

                            /* check if hostid is ok for a connect*/
                            if( (uuidcmp=GScale_Backend_Internal_CompareHostUUID(GScale_Backend_Internal_GetBinaryHostUUID(),
                            		                                             (const GScale_Host_UUID*)&packet.bcast.srchostuuid)) < 0){

                                /* can also be a v4 address, im using inet6 cause ipv6 has longer adresses so both will fit
                                 * +6 because the port number can be a string of 6 characters e.g.  ':65536'
                                 */
                            	socket = NULL;
								Try{
									switch(addr.ss_family){
									    case AF_INET:
									    	/* srcport is already in network byte order */
									    	((struct sockaddr_in*)&addr)->sin_port = packet.bcast.srcport;
									    	break;
										case AF_INET6:
											((struct sockaddr_in6*)&addr)->sin6_port = packet.bcast.srcport;
											break;
									}
                                    socket = GScale_Backend_BerkleySockets_Socket_connect(_this, &addr);
                                    GScale_Backend_Internal_CopyHostUUID((const GScale_Host_UUID*)&packet.bcast.srchostuuid, &socket->hostuuid);
                                    GScale_Backend_Berkleysockets_Database_updateSocket(_this->g->dbhandle, socket);
                                    if(socket->state == BERKLEYSOCKETS_STATE_CONNECTED){
                                        GScale_Backend_BerkleySockets_Util_SynchronizeSocket(_this, socket);
                                    }
								}
								Catch_anonymous{
									if(socket!=NULL){
		                        		GScale_Backend_BerkleySockets_Socket_destruct(_this, socket);
									}
								}
                            }
                            else if(uuidcmp!=0){
                                /* i'm not allowed to connect, so broadcast immediately to peer */
                                GScale_Backend_BerkleySockets_UDP_Broadcast(_this);
                            }

                        }

                    }
                    break;
                case SOCK_STREAM:
                    {
                        GScale_Backend_BerkleySockets_Socket *client = NULL;
                        Try{
                            client = GScale_Backend_BerkleySockets_Socket_accept(_this, socket);
                            GScale_Backend_BerkleySockets_Util_SynchronizeSocket(_this, client);
                            GSCALE_DEBUG("connection accepted");
                        }
                        Catch_anonymous{
                        	if(client!=NULL){
                        		GScale_Backend_BerkleySockets_Socket_destruct(_this, client);
                        	}
                        }
                    }
                    break;
            }
        }
        else{
           switch(socket->state){
               case BERKLEYSOCKETS_STATE_CONNECTING:
                   {
                       int error = 0;

                       socklen_t elen = sizeof(error);
                       getsockopt(socket->fd, SOL_SOCKET, SO_ERROR, &error, &elen);
                       if(error==0){
                           socket->state = BERKLEYSOCKETS_STATE_SYNCHRONIZING;
                       }
                       else{
                    	   GScale_Backend_BerkleySockets_Socket_destruct(_this, socket);
                    	   socket = NULL;
                           break;
                       }
                       /* now we're connected - so check if there is something to read */
                   }
               case BERKLEYSOCKETS_STATE_SYNCHRONIZING:
               {
                   GScale_Backend_BerkleySockets_Util_SynchronizeSocket(_this, socket);
                   if(socket->state==BERKLEYSOCKETS_STATE_SYNCHRONIZING){
                       /* synchronization not yet finished */
                       break;
                   }
               }
               case BERKLEYSOCKETS_STATE_READY:
               {
                   int32_t len = 0;
                   uint8_t validread = GSCALE_NO;

                   do{
                       /* read msg */
                       char buffer[8192];
                       errno = 0;

                       len = read(socket->fd, buffer, sizeof(buffer));

                       if(len>0){
                           validread = GSCALE_YES;
                           GScale_Backend_BerkleySockets_Util_ProcessReadBuffer(_this, socket, buffer, len);
                       }
                       /* an error can be seen in two situations:
                        * on first read() call we get len==0, equals, connection has been closed
                        * read returns -1 and errno is not temporary, equals, connection needs to be closed
                        */
                       else if((validread==GSCALE_NO && len==0) || (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR)){
                           GScale_Node *rnode = NULL;

                           /* since every socket is a unique way to a specific GScaleInstance on the peer identified by HOSTUUID
                            * we can be sure that every remote peer from this host is not reachable
                            * thus close socket and notify local peer's about nodeunavail
                            */

                           /* notification about remote peer down */
                           while((rnode=GScale_Backend_Internal_Nodes_nextByNode(_this->g, rnode, 1))!=NULL){
                               if(rnode->backend == _this && rnode->locality.remote.tag == socket){
                                   GScale_Backend_Internal_OnNodeUnavailable(_this, &rnode->locality.remote.id);
                               }
                           }
                           /* connection closed OR error, so close connection */
                           GScale_Backend_BerkleySockets_Socket_destruct(_this, socket);
                           socket = NULL;

                           break;
                       }
                   }while(len>0);
               }
               break;
           }
        }
    }
}

/*********************************** implementation functions **********************************************/

void
GScale_Backend_BerkleySockets_GetOption(GScale_Backend *_this, uint16_t optname,
                                        void *optval, uint16_t optlen)
{
    if (_this == NULL || (optval==NULL && optlen>0)) ThrowInvalidArgumentException();

    switch (optname) {
        case GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_PORT:
            break;
    }
}
void
GScale_Backend_BerkleySockets_SetOption(GScale_Backend *_this, uint16_t optname,
                                        const void *optval, uint16_t optlen)
{
    GScale_Backend_BerkleySockets_Class *instance = NULL;
    GScale_Backend_Berkleysockets_Options_CmdPort *cmd = NULL;

    if (_this == NULL) ThrowInvalidArgumentException();

    if(optlen>0 && optval==NULL){
        ThrowException("invalid optval VS optlen argument given for GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_BROADCASTPORT - non NULL optval expected", EINVAL);
    }

    instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;

    switch (optname) {
        case GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_PORT:
            if(optlen!=sizeof(GScale_Backend_Berkleysockets_Options_CmdPort) || optval==NULL){
                ThrowException("invalid optval/optlen argument given for GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_PORT - GScale_Backend_Berkleysockets_Options_CmdPort expected", EINVAL);
            }

            cmd = (GScale_Backend_Berkleysockets_Options_CmdPort*)optval;
            GScale_Backend_BerkleySockets_Util_OptionPort(_this, cmd);

            Try{
                GScale_Backend_BerkleySockets_Util_ProcessServers(_this);
                GScale_Backend_BerkleySockets_handleSockets(_this, NULL);
            }
            Catch_anonymous{}

            break;
            /* SOSNDBUF accepts a pointer to an uint32_t value containing send buffer size in byte */
        case GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_SOSNDBUF:
            if(optlen!=sizeof(uint32_t)){
                ThrowException("invalid optlen argument given for GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_SOSNDBUF - uint32_t expected", EINVAL);
            }
            instance->so_sndbuf = *(uint32_t*)optval;
            break;
            /* SORCVBUF accepts a pointer to an uint32_t value containing send buffer size in byte */
        case GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_SORCVBUF:
            if(optlen!=sizeof(uint32_t)){
                ThrowException("invalid optlen argument given for GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_SORCVBUF - uint32_t expected", EINVAL);
            }
            instance->so_rcvbuf = *(uint32_t*)optval;
            break;
            /* SOSNDTIMEO accepts a pointer to a timeval struct containing the timeout for send operations for the underlying sockets */
        case GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_SOSNDTIMEO:
            if(optlen!=sizeof(struct timeval)){
                ThrowException("invalid optlen argument given for GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_SOSNDTIMEO - struct timeval expected", EINVAL);
            }
            instance->so_sndtimeo = *(struct timeval*)optval;
            break;
            /* SORCVTIMEO accepts a pointer to a timeval struct containing the timeout for send operations for the underlying sockets */
        case GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_SORCVTIMEO:
            if(optlen!=sizeof(struct timeval)){
                ThrowException("invalid optlen argument given for GSCALE_BACKEND_BERKLEYSOCKETS_OPTION_SORCVTIMEO - struct timeval expected", EINVAL);
            }
            instance->so_rcvtimeo = *(struct timeval*)optval;
            break;
    }
}

void
GScale_Backend_BerkleySockets_Worker(GScale_Backend *_this, struct timeval *timeout)
{
    GScale_Backend_BerkleySockets_Class *instance = NULL;
    GScale_Backend_BerkleySockets_Socket* csocket = NULL;
    uint8_t enqueueagain = 0;
	time_t now = time(NULL);

    if (_this == NULL) ThrowInvalidArgumentException();

    instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;

    GScale_Backend_BerkleySockets_Util_ProcessServers(_this);
    timeout = NULL; /* noop */
    GScale_Backend_BerkleySockets_handleSockets(_this, timeout);

    if ((now - instance->lastbroadcast) > 60) {
        /* every 60 seconds send a broadcast packet */
    	GScale_Backend_BerkleySockets_UDP_Broadcast(_this);
        instance->lastbroadcast = now;
    }

    while((csocket = GScale_Backend_Berkleysockets_Database_getSocketBySettings(_this->g->dbhandle, csocket, -1, SOCK_STREAM, -1, 0, NULL))!=NULL){
        if(csocket->state != BERKLEYSOCKETS_STATE_READY){ continue; }

    	GScale_Backend_BerkleySockets_Util_ProcessMRSendBuffer(_this, csocket, csocket->eventreader);
        GScale_Backend_BerkleySockets_Util_ProcessMRSendBuffer(_this, csocket, csocket->sndreader);

        GScale_Backend_BerkleySockets_Util_ProcessSendBuffer(_this, csocket, &csocket->ringbuffer.internalsendbuffer);
        if(ringBuffer_canRead(&csocket->ringbuffer.internalsendbuffer)){
            enqueueagain = 1;
        }
    }

    if(enqueueagain || ringBuffer_canRead(instance->sndbuffer) || ringBuffer_canRead(instance->eventbuffer)){
        GScale_Backend_Internal_EnqueueWorker(_this->g);
    }

}

void
GScale_Backend_BerkleySockets_OnLocalNodeAvailable(GScale_Backend *_this, GScale_Node *node)
{
    GScale_Backend_BerkleySockets_Class *instance = NULL;

    if(_this==NULL || node==NULL) ThrowInvalidArgumentException();

    instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;
    if (instance == NULL) {
        ThrowException("backend not yet initialized", EINVAL);
    }

    if(GScale_Backend_Internal_CountLocalNodes(_this->g)<=1){
    	/* this line includes the condition to notify all
    	 * other nodes within reach that a new host is up */
        GScale_Backend_BerkleySockets_UDP_Broadcast(_this);
    }

    if(GScale_Database_CountTable(_this->g->dbhandle, "sockets", "server=0")<=0){
        /* no one is listening, so nothing to do */
        return;
    }

    /* send meta packets to tcp clients */

    /* seems localnode has connected
     * so we notify the other nodes
     */
    GScale_Backend_BerkleySockets_Util_EnqueueNodeAvailable(node, instance->eventbuffer);

    /* notify local worker to send the messages */
    GScale_Backend_Internal_EnqueueWorker(_this->g);
}

/**
 * OnNodeUnavail is only called if a node leaves the group - may be locally or remote node
 * in the end we need to notify remote nodes about this disconnect
 */
void
GScale_Backend_BerkleySockets_OnLocalNodeUnavailable(GScale_Backend *_this, GScale_Node *node)
{
    GScale_Backend_BerkleySockets_Class *instance = NULL;

    if(_this==NULL || node==NULL) ThrowInvalidArgumentException();

    instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;
    if (instance == NULL) {
        ThrowException("backend not yet initialized", EINVAL);
    }
    if(GScale_Database_CountTable(_this->g->dbhandle, "sockets", "server=0")<=0){
        /* no one is listening, so nothing to do */
        return;
    }
    /* send meta packets to tcp clients */

    /* seems localnode has disconnected
     * so we notify the other nodes
     */
    GScale_Backend_BerkleySockets_Util_EnqueueNodeUnavailble(node, instance->eventbuffer);

    /* notify local worker to send the messages */
    GScale_Backend_Internal_EnqueueWorker(_this->g);
}

int32_t
GScale_Backend_BerkleySockets_OnLocalNodeWritesToGroup(GScale_Backend *_this, GScale_Packet *packet)
{
    GScale_Backend_BerkleySockets_Message msg;
    GScale_Backend_BerkleySockets_Class *instance = NULL;
    uint32_t fionlen = 0;
    int32_t writelength = 0;
    uint32_t datasize = 0;
    void *ptr[3] = { NULL, NULL };
    int32_t lengths[3] = { 0, 0, 0 };

    if(_this==NULL || packet==NULL) ThrowInvalidArgumentException();

    instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;
    if (instance == NULL) {
        ThrowException("backend not yet initialized", EINVAL);
    }

    if(GScale_Database_CountTable(_this->g->dbhandle, "sockets", "server=0")<=0){
        /* no one is listening, so nothing to do */
        return 0;
    }

    msg.header.type.nodeavail = GSCALE_NOK;
    msg.header.type.nodeunavail = GSCALE_NOK;
    msg.header.type.data |= GSCALE_OK;
    msg.payload.hdr = packet->header;
    msg.payload.hdr.packetid = htonl(msg.payload.hdr.packetid);

    /* enqueue the message into ringbuffer */
    fionlen = ringBuffer_getFIONWRITE(instance->sndbuffer);
    if(fionlen>sizeof(msg)){
        datasize = fionlen - sizeof(msg);
        /* reduce if packet data length is less than whats possible */
        if(datasize > packet->data.length){
            datasize = packet->data.length;
        }
        msg.header.length = htonl(datasize);

        ptr[0] = &msg;
        ptr[1] = packet->data.buffer;
        lengths[0] = sizeof(msg);
        lengths[1] = datasize;

        ringBuffer_multiWriteExt(instance->sndbuffer, ptr, lengths, GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);
        writelength = datasize;
    }

    /* notify local worker to send the messages */
    GScale_Backend_Internal_EnqueueWorker(_this->g);

    return writelength;
}

void
GScale_Backend_BerkleySockets_Shutdown(GScale_Backend *_this)
{
    struct timeval timeout = {0, 100000};

    if (_this == NULL) ThrowInvalidArgumentException();

    GScale_Backend_BerkleySockets_Worker(_this, &timeout);

    GScale_Backend_BerkleySockets_Util_ShutdownBackend(_this);
}

/**
 * mcast:
 * 239.255.92.19
 * ff00:0:0:0:0:0:efff:5c13
 *
 */

void
GScale_Backend_BerkleySockets_Init(GScale_Backend *_this)
{
    GScale_Backend_BerkleySockets_Class *instance = NULL;
    uint32_t buffersize = 0;
    GScale_Exception except;
    struct sockaddr_storage saddr;
    int servport = 8000;

    if (_this == NULL) ThrowInvalidArgumentException();

    Try{
        _this->backend_private = calloc(1, sizeof(GScale_Backend_BerkleySockets_Class));

        instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;
        if (instance == NULL) ThrowException("invalid instance", EINVAL);

        GScale_Backend_Berkleysockets_Database_Init(_this->g->dbhandle);

        ThrowErrnoOn((instance->evserversockets=epoll_create(512))==-1);
        GScale_Backend_Internal_AddEventDescriptor(_this->g, instance->evserversockets, EPOLLIN);

        ThrowErrnoOn((instance->evsockets=epoll_create(512))==-1);
        GScale_Backend_Internal_AddEventDescriptor(_this->g, instance->evsockets, EPOLLIN);

        instance->lastbroadcast = time(NULL) - 61; /* 61 just to be more than 60 seconds in the past*/
        buffersize = GScale_Backend_BerkleySockets_Netspeed_getMaxBufferSize(64*1024);
        {
            ThrowErrnoOn((instance->sndbuffer = calloc(1, buffersize + sizeof(ringBuffer)))==NULL);

            ringBuffer_initMetaPointer(instance->sndbuffer, &instance->sndmeta);
            ringBuffer_initDataPointer(instance->sndbuffer, buffersize, NULL);
            ringBuffer_reset(instance->sndbuffer);
            mrRingBuffer_init(&instance->sndmrmeta, instance->sndbuffer);
        }
        {
            ThrowErrnoOn((instance->eventbuffer = calloc(1, 1024*sizeof(GScale_Backend_BerkleySockets_Message)+1))==NULL);

            ringBuffer_initMetaPointer(instance->eventbuffer, &instance->eventmeta);
            ringBuffer_initDataPointer(instance->eventbuffer, buffersize, NULL);
            ringBuffer_reset(instance->eventbuffer);
            mrRingBuffer_init(&instance->eventmrmeta, instance->eventbuffer);
        }

        _this->ioc.OnLocalNodeAvailable = &GScale_Backend_BerkleySockets_OnLocalNodeAvailable;
        _this->ioc.OnLocalNodeUnavailable = &GScale_Backend_BerkleySockets_OnLocalNodeUnavailable;

        _this->ioc.OnLocalNodeWritesToGroup
                = &GScale_Backend_BerkleySockets_OnLocalNodeWritesToGroup;

        _this->instance.Worker = &GScale_Backend_BerkleySockets_Worker;
        _this->instance.GetOption = &GScale_Backend_BerkleySockets_GetOption;
        _this->instance.SetOption = &GScale_Backend_BerkleySockets_SetOption;
        _this->instance.Shutdown = &GScale_Backend_BerkleySockets_Shutdown;

        ((struct sockaddr_in*)&saddr)->sin_family = AF_INET;
        ((struct sockaddr_in*)&saddr)->sin_port = htons(8000);
        inet_pton(AF_INET, "239.255.92.19", &(((struct sockaddr_in*)&saddr)->sin_addr));
        instance->v4mcast = GScale_Backend_BerkleySockets_Socket_createMulticastReceiver(_this, &saddr);
        instance->v4_dgram_socket = -1;
        GScale_Backend_BerkleySockets_Socket_create(instance, AF_INET, SOCK_DGRAM, &instance->v4_dgram_socket);

        ((struct sockaddr_in6*)&saddr)->sin6_family = AF_INET6;
        ((struct sockaddr_in6*)&saddr)->sin6_port = htons(8000);
        inet_pton(AF_INET6, "ff00:0:0:0:0:0:efff:5c13", &(((struct sockaddr_in6*)&saddr)->sin6_addr));
        instance->v6mcast = GScale_Backend_BerkleySockets_Socket_createMulticastReceiver(_this, &saddr);
        instance->v6_dgram_socket = -1;
        GScale_Backend_BerkleySockets_Socket_create(instance, AF_INET6, SOCK_DGRAM, &instance->v6_dgram_socket);

        for(servport=8000;servport<9000;servport++){
        	GScale_Backend_BerkleySockets_Socket *tmpsock = NULL;

            Try{
                ((struct sockaddr_in*)&saddr)->sin_family = AF_INET;
                ((struct sockaddr_in*)&saddr)->sin_port = htons(servport);
                ((struct sockaddr_in*)&saddr)->sin_addr.s_addr = INADDR_ANY;
                tmpsock = GScale_Backend_BerkleySockets_Socket_createServer(_this, SOCK_STREAM, &saddr);
                ((struct sockaddr_in6*)&saddr)->sin6_family = AF_INET6;
                ((struct sockaddr_in6*)&saddr)->sin6_port = htons(servport);
                ((struct sockaddr_in6*)&saddr)->sin6_addr = in6addr_any;
                GScale_Backend_BerkleySockets_Socket_createServer(_this, SOCK_STREAM, &saddr);

                servport = 9000;
            }Catch_anonymous{
            	if(tmpsock!=NULL){
            		GScale_Backend_BerkleySockets_Socket_destruct(_this, tmpsock);
            	}
            }
        }    }
    Catch(except){
        GScale_Backend_BerkleySockets_Util_ShutdownBackend(_this);

        errno = except.code;
        Throw except;
    }
}

#endif /* BERKLEYSOCKETS_H_ */
