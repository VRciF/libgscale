#ifndef BERKLEYSOCKETS_UTIL_C_
#define BERKLEYSOCKETS_UTIL_C_

#include <stddef.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ifaddrs.h>

#include "exception.h"
#include "debug.h"
#include "util/log.h"
#include "../../util.h"
#include "backend/internal.h"
#include "backend/berkleysockets/util.h"
#include "backend/berkleysockets/database.h"
#include "backend/berkleysockets/socket.h"
#include "backend/berkleysockets.h"

/* following function from beejs network programming guide (http://beej.us/guide/bgnet/output/html/multipage/inet_ntopman.html)
 * many thanks to Brian "Beej Jorgensen" Hall
 */
char*
GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr(const struct sockaddr_storage *sa) /* throw (GScale_Exception) */
{
	static char ip[NI_MAXHOST+1] = {'\0'};
	return GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr_r(sa, ip, sizeof(ip));
}
char*
GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr_r(const struct sockaddr_storage *sa, char *s, size_t maxlen) /* throw (GScale_Exception) */
{
    if(maxlen<=0){
        ThrowException("invalid maxlen given", EINVAL);
    }
    if(sa==NULL || s==NULL){
        ThrowException("invalid sockaddr or string ptr given", EINVAL);
    }

    memset(s, '\0', maxlen);
    switch(sa->ss_family) {
        case AF_INET:
            ThrowErrnoOn(inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen)==NULL);
            break;
        case AF_INET6:
            ThrowErrnoOn(inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen)==NULL);
            break;
        default:
            ThrowException("invalid family given", EINVAL);
    }

/*
         if (ifa->ifa_addr->sa_family == AF_INET || ifa->ifa_addr->sa_family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                    (ifa->ifa_addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) :
                                          sizeof(struct sockaddr_in6),
                    ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                ThrowException(gai_strerror(s), errno);
            }
        }
*/

    return s;
}
struct sockaddr_storage*
GScale_Backend_BerkleySockets_Util_GetSockaddrFromIp(int af, const char *src, struct sockaddr_storage *dest){
    if(src==NULL || dest==NULL){
        ThrowException("invalid sockaddr or ip-ptr given", EINVAL);
    }

    switch(af){
        case AF_INET:
        	inet_pton(AF_INET, src, &(((struct sockaddr_in *)dest)->sin_addr));
            break;
        case AF_INET6:
        	inet_pton(AF_INET6, src, &(((struct sockaddr_in6 *)dest)->sin6_addr));
            break;
    }
    return dest;
}

uint16_t
GScale_Backend_BerkleySockets_Util_GetPortFromSockaddr(const struct sockaddr_storage *sa) /* throw (GScale_Exception) */
{
    uint16_t port = 0;

    if(sa==NULL){
        ThrowException("invalid sockaddr or string ptr given", EINVAL);
    }

    switch(sa->ss_family) {
        case AF_INET:
        	port = htons(((struct sockaddr_in *)sa)->sin_port);
            break;
        case AF_INET6:
        	port = htons(((struct sockaddr_in6 *)sa)->sin6_port);
            break;
        default:
            ThrowException("invalid family given", EINVAL);
    }

    return port;
}

struct sockaddr_storage* GScale_Backend_BerkleySockets_Util_IfaceToSockaddr(const char *ifname, struct sockaddr_storage *sa) /* throw (GScale_Exception) */ {
    struct ifaddrs *ifaddr = NULL, *ifa=NULL;

    if(ifname==NULL || sa==NULL){
        ThrowException("invalid ifname or storage given", EINVAL);
    }

    if (getifaddrs(&ifaddr) == -1) {
        ThrowException("get interfaces failed", errno);
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if(ifa->ifa_name!=NULL && strcmp(ifname, ifa->ifa_name)!=0){
            continue;
        }

        switch(ifa->ifa_addr->sa_family){
            case AF_INET:
                memcpy(sa, &ifa->ifa_addr, sizeof(struct sockaddr_in));
                break;
            case AF_INET6:
                memcpy(sa, &ifa->ifa_addr, sizeof(struct sockaddr_in6));
                break;
            default:
                ThrowException("invalid family given", EINVAL);
        }
        break;
    }
    freeifaddrs(ifaddr);

    return sa;
}


uint32_t
GScale_Backend_BerkleySockets_Util_ProcessMRSendBuffer(GScale_Backend *_this,
                                                       GScale_Backend_BerkleySockets_Socket *socket,
                                                       mrRingBufferReader *reader) /* throw (GScale_Exception) */
{
    uint32_t written = 0;
    ringBuffer *rb = NULL;

    if(_this==NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    /* using ringBuffer DMA access we try to write as much as possible */
    mrRingBuffer_switchToReader(reader);
    rb = mrRingBuffer_getRingBuffer(reader);

    written = GScale_Backend_BerkleySockets_Util_ProcessSendBuffer(_this, socket, rb);

    mrRingBuffer_switchBackToRingBuffer(reader);

    return written;
}

uint32_t
GScale_Backend_BerkleySockets_Util_ProcessSendBuffer(GScale_Backend *_this,
                                                    GScale_Backend_BerkleySockets_Socket *socket,
                                                    ringBuffer *rb) /* throw (GScale_Exception) */
{
    int32_t length = 0;
    uint32_t written = 0;
    void *data = NULL;

    if(_this==NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    while((length = ringBuffer_dmaRead(rb, &data))>0){
        /* at this point: length = length which can be read */
        length = send(socket->fd, data, length, MSG_NOSIGNAL);

        /* at this point: result = length how many bytes have been written */
        if(length <= 0){
            GScale_Backend_BerkleySockets_Socket_enableEPOLLOUT(_this, socket);
            break;
        }

        written += length;
        ringBuffer_readExt(rb, NULL, length, GSCALE_RINGBUFFER_OPTION_SKIPDATA);
    }

    return written;
}

uint32_t
GScale_Backend_BerkleySockets_Util_ProcessReadBuffer(GScale_Backend *_this,
                                                     GScale_Backend_BerkleySockets_Socket *socket,
                                                     char *buffer, uint32_t len)
{
    uint32_t nread = 0;
    uint32_t pending = 0;
    GScale_Packet message;

    while(len>0){
        if(socket->msgoffset<sizeof(socket->currentmsg)){
            pending = sizeof(socket->currentmsg)-socket->msgoffset;
            if(len<pending){
                pending = len;
            }
            memcpy(((char*)&socket->currentmsg)+socket->msgoffset, buffer+nread, pending);
            len -= pending;
            nread += pending;

            socket->msgoffset += pending;
            if(socket->msgoffset==sizeof(socket->currentmsg)){
                socket->currentmsg.header.length = ntohl(socket->currentmsg.header.length);
                socket->currentmsg.payload.hdr.packetid = ntohl(socket->currentmsg.payload.hdr.packetid);
            }
        }

        if(socket->msgoffset != sizeof(socket->currentmsg)){
            continue;
        }

        /* at this point we got a message */
        socket->currentmsg.header.uptimesec = GScale_Util_ntohll(socket->currentmsg.header.uptimesec);
        socket->currentmsg.header.uptimeusec = GScale_Util_ntohll(socket->currentmsg.header.uptimeusec);

        if (socket->currentmsg.header.type.nodeavail) {
            if(socket->flags.msgrcvd == GSCALE_NO){
                GScale_Backend_Internal_CopyHostUUID((const GScale_Host_UUID*)&socket->currentmsg.payload.id.hostuuid, &socket->hostuuid);
                GScale_Backend_Berkleysockets_Database_updateSocket(_this->g->dbhandle, socket);
            }

            GScale_Backend_Internal_OnNodeAvailable(_this, &socket->currentmsg.payload.id, socket);
            socket->flags.msgrcvd |= GSCALE_YES;
            socket->msgoffset = 0;
        } else if(socket->currentmsg.header.type.nodeunavail){
            if(socket->flags.msgrcvd == GSCALE_NO){
                GScale_Backend_Internal_CopyHostUUID((const GScale_Host_UUID*)&socket->currentmsg.payload.id.hostuuid, &socket->hostuuid);
                GScale_Backend_Berkleysockets_Database_updateSocket(_this->g->dbhandle, socket);
            }

            GScale_Backend_Internal_OnNodeUnavailable(_this, &socket->currentmsg.payload.id);

            socket->flags.msgrcvd |= GSCALE_YES;
            socket->msgoffset = 0;
        }
        else{
            if(socket->flags.msgrcvd == GSCALE_NO){
                GScale_Backend_Internal_CopyHostUUID((const GScale_Host_UUID*)&socket->currentmsg.payload.hdr.src.hostuuid, &socket->hostuuid);
                GScale_Backend_Berkleysockets_Database_updateSocket(_this->g->dbhandle, socket);
            }

            /* this is a data message and we may have pending data */
            message.header = socket->currentmsg.payload.hdr;

            message.data.buffer = buffer+nread;
            pending = socket->currentmsg.header.length;
            if(len < pending){
                pending = len;
            }
            message.data.length = pending;

            nread += pending;
            len -= pending;
            socket->currentmsg.header.length -= pending;

            if(socket->currentmsg.header.length<=0){
                socket->msgoffset = 0;
            }
            if(socket->currentmsg.header.type.data){
                GScale_Backend_Internal_SendLocalMessage(_this, &message);
            }
            socket->flags.msgrcvd |= GSCALE_YES;
        }
    }

    return nread;
}

void
GScale_Backend_BerkleySockets_Util_EnqueueNodeAvailable(GScale_Node *node, ringBuffer *rb){
    GScale_Backend_BerkleySockets_Message msg;
    uint32_t fionlen = 0;
    void *ptr[2] = { NULL, NULL };
    int32_t lengths[2] = { sizeof(msg), 0 };
    struct timeval tv;

    gettimeofday(&tv,NULL);
    tv = GScale_Util_Timeval_subtract(tv, node->added);
    msg.header.uptimesec = GScale_Util_htonll(tv.tv_sec);
    msg.header.uptimeusec = GScale_Util_htonll(tv.tv_usec);

    msg.header.type.nodeavail |= GSCALE_OK;
    msg.header.type.nodeunavail = GSCALE_NOK;
    msg.header.type.data = 0;
    msg.header.length = sizeof(msg.payload.id);
    GScale_Backend_Internal_SetFQNodeIdentifierByNode(node, &msg.payload.id);

    /* enqueue the message into ringbuffer */
    fionlen = ringBuffer_getFIONWRITE(rb);
    if(fionlen>=sizeof(msg)){
        msg.header.length = 0;
        ptr[0] = &msg;
        ringBuffer_multiWriteExt(rb, ptr, lengths, GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);
    }
}
void
GScale_Backend_BerkleySockets_Util_EnqueueNodeUnavailble(GScale_Node *node, ringBuffer *rb){
    GScale_Backend_BerkleySockets_Message msg;
    uint32_t fionlen = 0;
    void *ptr[2] = { NULL, NULL };
    int32_t lengths[2] = { sizeof(msg), 0 };
    struct timeval tv;

    gettimeofday(&tv,NULL);
    tv = GScale_Util_Timeval_subtract(tv, node->added);
    msg.header.uptimesec = GScale_Util_htonll(tv.tv_sec);
    msg.header.uptimeusec = GScale_Util_htonll(tv.tv_usec);

    msg.header.type.nodeavail = GSCALE_NOK;
    msg.header.type.nodeunavail |= GSCALE_OK;
    msg.header.type.data = 0;
    msg.header.length = sizeof(msg.payload.id);
    GScale_Backend_Internal_SetFQNodeIdentifierByNode(node, &msg.payload.id);

    /* enqueue the message into ringbuffer */
    fionlen = ringBuffer_getFIONWRITE(rb);
    if(fionlen>sizeof(msg)){
        msg.header.length = 0;
        ptr[0] = &msg;
        ringBuffer_multiWriteExt(rb, ptr, lengths, GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);
    }
}

void GScale_Backend_BerkleySockets_Util_SynchronizeSocket(GScale_Backend *_this, GScale_Backend_BerkleySockets_Socket *socket){
    GScale_Node nullnode = GSCALE_INIT_NODE;
    GScale_Node *localnode = NULL;

    socket->state = BERKLEYSOCKETS_STATE_SYNCHRONIZING;

    gettimeofday(&nullnode.added, NULL);
    GScale_Backend_BerkleySockets_Util_EnqueueNodeAvailable(&nullnode, &socket->ringbuffer.internalsendbuffer);

    while((localnode = GScale_Backend_Internal_Nodes_nextByAddedTime(_this->g, &socket->synctime, 0))!=NULL){
        GScale_Backend_BerkleySockets_Util_EnqueueNodeAvailable(localnode, &socket->ringbuffer.internalsendbuffer);
        socket->synctime = localnode->added;
    }
    if(localnode==NULL){
        socket->state = BERKLEYSOCKETS_STATE_READY;
    }
    GScale_Backend_Internal_EnqueueWorker(_this->g);
}

void GScale_Backend_BerkleySockets_Util_ShutdownBackend(GScale_Backend *_this) /* throw (GScale_Exception) */{
    GScale_Backend_BerkleySockets_Class *instance = NULL;
    GScale_Backend_BerkleySockets_Socket *tmpsocket = NULL;

    instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;
    if (instance == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    while((tmpsocket=GScale_Backend_Berkleysockets_Database_getSocketBySettings(_this->g->dbhandle, NULL, -1, -1 , -1, -1, NULL))!=NULL){
        GScale_Backend_BerkleySockets_Socket_destruct(_this, tmpsocket);
    }

    if(instance->evserversockets>0){
        GScale_Backend_Internal_RemoveEventDescriptor(_this->g, instance->evserversockets);
        close(instance->evserversockets);
    }
    if(instance->evsockets>0){
        GScale_Backend_Internal_RemoveEventDescriptor(_this->g, instance->evsockets);
        close(instance->evsockets);
    }
    if(instance->sndbuffer!=NULL){
        mrRingBuffer_free(&instance->sndmrmeta);
        free(instance->sndbuffer);
    }
    if(instance->eventbuffer!=NULL){
        mrRingBuffer_free(&instance->eventmrmeta);
        free(instance->eventbuffer);
    }

    if(instance->pipefds[0]>0){
        close(instance->pipefds[0]);
    }
    if(instance->pipefds[1]>0){
        close(instance->pipefds[1]);
    }

    if(_this->backend_private!=NULL){
        free(_this->backend_private);
        _this->backend_private = NULL;
    }
}

void GScale_Backend_BerkleySockets_Util_ProcessServers(GScale_Backend *_this){
    GScale_FutureResult fresult;
    struct sockaddr_storage saddr;
    int32_t prevservid=-1;
    int type = 0;

    if (_this == NULL) ThrowInvalidArgumentException();

    memset(&saddr, '\0', sizeof(struct sockaddr_storage));
    while(GScale_Backend_Berkleysockets_Database_getNextMissingServer(_this->g->dbhandle, prevservid, &saddr, &type, &fresult)>=0){
        GScale_Exception except;

        Try{
            GScale_Backend_BerkleySockets_Socket_createServer(_this, type, &saddr);
            fresult.success = GSCALE_OK;
        }Catch(except){
            fresult.success = GSCALE_NOK;
            fresult.except = except;
        }
        fresult.resultcallback(&fresult, _this->g, GScale_Backend_BerkleySockets);
    }
}

void GScale_Backend_BerkleySockets_Util_OptionPort(GScale_Backend *_this, GScale_Backend_Berkleysockets_Options_CmdPort *cmd){
    GScale_Exception except;
    uint8_t includelo = 0;
    struct sockaddr_storage in;
    struct sockaddr_in *inp = NULL;
    struct sockaddr_in6 *in6p = NULL;

    if(cmd->addr.ss_family!=AF_UNSPEC && cmd->addr.ss_family!=AF_INET && cmd->addr.ss_family!=AF_INET6){
        ThrowException("invalid domain/family given", EINVAL);
    }
    if((!cmd->flags.add && !cmd->flags.del) || cmd->flags.add==cmd->flags.del){
        ThrowException("either use flags.add OR flags.del - both cant be set", EINVAL);
    }

    Try{
        sqlite3_exec(_this->g->dbhandle, "BEGIN", 0,0,0);
        if(cmd->flags.listen_stream || cmd->flags.listen_dgram){
        	includelo = 0;
            /* this means bind on a port to listen too */
            if(cmd->addr.ss_family!=AF_INET6){
                /* handling AF_INET ports */
                inp = (struct sockaddr_in*)&in;
                memcpy(inp, &cmd->addr, sizeof(struct sockaddr_storage));
                if(cmd->addr.ss_family==AF_UNSPEC){
                	/*includelo = 1;*/
                    inp->sin_addr.s_addr = INADDR_ANY;
                    inp->sin_port = ((struct sockaddr_in*)&cmd->addr)->sin_port;
                }

                inp->sin_family = AF_INET;
                if(cmd->flags.add){
                    if(cmd->flags.listen_stream){
                        GScale_Backend_Berkleysockets_Database_insertPort(_this->g->dbhandle, &in, SOCK_STREAM, 1, cmd->fr);
                    }
                    if(cmd->flags.listen_dgram){
                        GScale_Backend_Berkleysockets_Database_insertPort(_this->g->dbhandle, &in, SOCK_DGRAM, 1, cmd->fr);
                    }
                    if(includelo){
                    	inet_pton(AF_INET, "127.0.0.1", &inp->sin_addr);
                        if(cmd->flags.listen_stream){
                            GScale_Backend_Berkleysockets_Database_insertPort(_this->g->dbhandle, &in, SOCK_STREAM, 1, cmd->fr);
                        }
                        if(cmd->flags.listen_dgram){
                            GScale_Backend_Berkleysockets_Database_insertPort(_this->g->dbhandle, &in, SOCK_DGRAM, 1, cmd->fr);
                        }
                    }
                }
                else if(cmd->flags.del){
                    if(cmd->flags.listen_stream){
                        GScale_Backend_Berkleysockets_Database_deletePort(_this->g->dbhandle, cmd->id, &in, SOCK_STREAM, 1);
                    }
                    if(cmd->flags.listen_dgram){
                        GScale_Backend_Berkleysockets_Database_deletePort(_this->g->dbhandle, cmd->id, &in, SOCK_DGRAM, 1);
                    }
                    if(includelo){
                    	inet_pton(AF_INET, "127.0.0.1", &inp->sin_addr);
                        if(cmd->flags.listen_stream){
                            GScale_Backend_Berkleysockets_Database_deletePort(_this->g->dbhandle, cmd->id, &in, SOCK_STREAM, 1);
                        }
                        if(cmd->flags.listen_dgram){
                            GScale_Backend_Berkleysockets_Database_deletePort(_this->g->dbhandle, cmd->id, &in, SOCK_DGRAM, 1);
                        }
                    }
                }
            }
            includelo = 0;
            if(cmd->addr.ss_family!=AF_INET){
                /* handling AF_INET6 ports */
                in6p = (struct sockaddr_in6*)&in;
                memcpy(in6p, &cmd->addr, sizeof(struct sockaddr_storage));

                if(cmd->addr.ss_family==AF_UNSPEC){
                	/*includelo = 1;*/
                    in6p->sin6_addr = in6addr_any;
                    in6p->sin6_port = ((struct sockaddr_in*)&cmd->addr)->sin_port;
                }

                in6p->sin6_family = AF_INET6;
                if(cmd->flags.add){
                    if(cmd->flags.listen_stream){
                        GScale_Backend_Berkleysockets_Database_insertPort(_this->g->dbhandle, &in, SOCK_STREAM, 1, cmd->fr);
                    }
                    if(cmd->flags.listen_dgram){
                        GScale_Backend_Berkleysockets_Database_insertPort(_this->g->dbhandle, &in, SOCK_DGRAM, 1, cmd->fr);
                    }
                    if(includelo){
                    	inet_pton(AF_INET6, "::1", &in6p->sin6_addr);
                        if(cmd->flags.listen_stream){
                            GScale_Backend_Berkleysockets_Database_insertPort(_this->g->dbhandle, &in, SOCK_STREAM, 1, cmd->fr);
                        }
                        if(cmd->flags.listen_dgram){
                            GScale_Backend_Berkleysockets_Database_insertPort(_this->g->dbhandle, &in, SOCK_DGRAM, 1, cmd->fr);
                        }
                    }
                }
                else if(cmd->flags.del){
                    if(cmd->flags.listen_stream){
                        GScale_Backend_Berkleysockets_Database_deletePort(_this->g->dbhandle, cmd->id, &in, SOCK_STREAM, 1);
                    }
                    if(cmd->flags.listen_dgram){
                        GScale_Backend_Berkleysockets_Database_deletePort(_this->g->dbhandle, cmd->id, &in, SOCK_DGRAM, 1);
                    }
                    if(includelo){
                    	inet_pton(AF_INET6, "::1", &in6p->sin6_addr);
                        if(cmd->flags.listen_stream){
                            GScale_Backend_Berkleysockets_Database_deletePort(_this->g->dbhandle, cmd->id, &in, SOCK_STREAM, 1);
                        }
                        if(cmd->flags.listen_dgram){
                            GScale_Backend_Berkleysockets_Database_deletePort(_this->g->dbhandle, cmd->id, &in, SOCK_DGRAM, 1);
                        }
                    }
                }
            }
        }

        includelo = 0;
        if(cmd->flags.broadcast){
            inp = (struct sockaddr_in*)&in;
            memcpy(&in, &cmd->addr, sizeof(struct sockaddr_storage));

            if(cmd->addr.ss_family==AF_UNSPEC){
            	includelo = 1;
                inet_pton(AF_INET, "255.255.255.255", &inp->sin_addr);
                inp->sin_port = ((struct sockaddr_in*)&cmd->addr)->sin_port;
                inp->sin_family = AF_INET;
            }
            if(cmd->flags.add){
                 GScale_Backend_Berkleysockets_Database_insertPort(_this->g->dbhandle, &in, SOCK_DGRAM, 0, cmd->fr);
                 if(includelo){
					 inet_pton(AF_INET, "127.0.0.1", &inp->sin_addr);
					 GScale_Backend_Berkleysockets_Database_insertPort(_this->g->dbhandle, &in, SOCK_DGRAM, 0, cmd->fr);
                 }
            }
            else if(cmd->flags.del){
                 GScale_Backend_Berkleysockets_Database_deletePort(_this->g->dbhandle, cmd->id, &in, SOCK_DGRAM, 0);
                 if(includelo){
					 inet_pton(AF_INET, "127.0.0.1", &inp->sin_addr);
					 GScale_Backend_Berkleysockets_Database_deletePort(_this->g->dbhandle, cmd->id, &in, SOCK_DGRAM, 0);
                 }
            }
        }
        sqlite3_exec(_this->g->dbhandle, "COMMIT", 0,0,0);
    }
    Catch(except){
        sqlite3_exec(_this->g->dbhandle, "ROLLBACK", 0,0,0);
        Throw except;
    }
}

#endif
