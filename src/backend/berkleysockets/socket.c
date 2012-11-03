#ifndef BERKLEYSOCKETS_SOCKETOPS_C_
#define BERKLEYSOCKETS_SOCKETOPS_C_

#include <fcntl.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <sys/epoll.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>

#include "util/log.h"
#include "backend/berkleysockets/netspeed.h"
#include "backend/berkleysockets/util.h"
#include "backend/berkleysockets/socket.h"
#include "backend/berkleysockets/database.h"
#include "exception.h"
#include "debug.h"

void
GScale_Backend_BerkleySockets_Socket_create(GScale_Backend_BerkleySockets_Class *instance,
                                           const int domain, const int type,
                                           int *sockfd)
{
    int iyes = 1;
    int cyes = '1';
    /*int ino = 0;
    int cno = '0';
    */
#if defined( IPV6_V6ONLY )
    int v6only = 1;
#endif
    uint32_t sndbuffersize = 0;
    uint32_t rcvbuffersize = 0;
    uint32_t maxbuffersize = 0;

    if(sockfd == NULL) ThrowInvalidArgumentException();
    ThrowErrnoOn((*sockfd = socket(domain, type, 0)) == -1);

    if(type==SOCK_DGRAM){
        if(setsockopt(*sockfd, SOL_SOCKET, SO_BROADCAST, &iyes, sizeof(iyes))==-1){
            setsockopt(*sockfd, SOL_SOCKET, SO_BROADCAST, &cyes, sizeof(cyes));
        }
    }
#if defined( IPV6_V6ONLY )
        if ( domain == AF_INET6 )
        {
            /*
            ** Disable IPv4 mapped addresses.
            */
            setsockopt( *sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));
        }
#endif

    /* calculate the send/recv buffer size for current used network interfaces */
    sndbuffersize = instance->so_sndbuf;
    rcvbuffersize = instance->so_rcvbuf;

    /* the following netspeed calculation is called for every newly created socket
     * im not caching the result in some static var cause it could be that e.g. a wireless interface
     * comes up which increases buffer size - so in general its not sure that the network interfaces are
     * the same during runtime
     */
    maxbuffersize = GScale_Backend_BerkleySockets_Netspeed_getMaxBufferSize(1);
    if(sndbuffersize<=0){
        sndbuffersize = maxbuffersize;
    }
    if(rcvbuffersize<=0){
        rcvbuffersize = maxbuffersize;
    }
    if(sndbuffersize>0){
        setsockopt( *sockfd, SOL_SOCKET, SO_SNDBUF, (void *)&sndbuffersize, sizeof(sndbuffersize));
    }
    if(rcvbuffersize>0){
        setsockopt( *sockfd, SOL_SOCKET, SO_RCVBUF, (void *)&rcvbuffersize, sizeof(rcvbuffersize));
    }
#ifdef SO_NOSIGPIPE
    if(setsockopt(*sockfd, SOL_SOCKET, SO_NOSIGPIPE, &iyes, sizeof(iyes))==-1){
        setsockopt(*sockfd, SOL_SOCKET, SO_NOSIGPIPE, &cyes, sizeof(cyes));
    }
#endif

    /* set socket timeouts */
    if(instance->so_sndtimeo.tv_sec>0 || instance->so_sndtimeo.tv_usec>0){
        setsockopt( *sockfd, SOL_SOCKET, SO_SNDTIMEO, (void *)&instance->so_sndtimeo, sizeof(instance->so_sndtimeo));
    }
    if(instance->so_rcvtimeo.tv_sec>0 || instance->so_rcvtimeo.tv_usec>0){
        setsockopt( *sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&instance->so_rcvtimeo, sizeof(instance->so_rcvtimeo));
    }

    /* lose the pesky "Address already in use" error message */
    if (setsockopt(*sockfd,SOL_SOCKET,SO_REUSEADDR,&iyes,sizeof(int)) == -1) {
        setsockopt(*sockfd,SOL_SOCKET,SO_REUSEADDR,&cyes,sizeof(char));
    }

    /* disable multicast loopback */
    /*
    if(setsockopt(*sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &ino, sizeof(int)) == -1){
        setsockopt(*sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &cno, sizeof(char));
    }
    */
}

GScale_Backend_BerkleySockets_Socket*
GScale_Backend_BerkleySockets_Socket_connect(GScale_Backend *_this,
                                                    struct sockaddr_storage *addr)
{
    GScale_Exception except;
    GScale_Backend_BerkleySockets_Class *instance = NULL;

    size_t addrlen = sizeof(struct sockaddr_storage);
    int sockfd = -1;
    int state = BERKLEYSOCKETS_STATE_CONNECTED;
    int port = 0;
    GScale_Backend_BerkleySockets_Socket *socket = NULL;

    if(_this==NULL || _this->backend_private==NULL || addr==NULL) ThrowInvalidArgumentException();

    switch(addr->ss_family){
		case AF_INET:
			addrlen = sizeof(struct sockaddr_in);
			break;
		case AF_INET6:
			addrlen = sizeof(struct sockaddr_in6);
			break;
    }

    instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;

    GScale_Backend_BerkleySockets_Socket_create(instance, addr->ss_family, SOCK_STREAM, &sockfd);

    errno = 0;
    if(connect(sockfd, (struct sockaddr*)addr, addrlen) == -1){
        if(errno==EINPROGRESS) {
            state = BERKLEYSOCKETS_STATE_CONNECTING;
        }
        else{
            close(sockfd);
            sockfd = -1;
            ThrowException(strerror(errno), errno);
        }
    }

    switch(addr->ss_family){
		case AF_INET:
			port = ntohs(((struct sockaddr_in *)&addr)->sin_port);
			break;
		case AF_INET6:
			port = ntohs(((struct sockaddr_in6 *)&addr)->sin6_port);
			break;
    }

    socket = NULL;
    Try{
        socket = GScale_Backend_BerkleySockets_Socket_construct(_this, sockfd, GSCALE_NO, SOCK_STREAM, addr->ss_family, state, port);

        socket->sndreader = mrRingBuffer_initReader(&instance->sndmrmeta);
        socket->eventreader = mrRingBuffer_initReader(&instance->eventmrmeta);
    }
    Catch(except){
        if(socket!=NULL){
            GScale_Backend_BerkleySockets_Socket_destruct(_this, socket);
        }
        else{
            close(sockfd);
            sockfd = -1;
            /*socket = NULL;*/
        }
        Throw(except);
    }
    return socket;
}

/*SOCK_STREAM*/
GScale_Backend_BerkleySockets_Socket*
GScale_Backend_BerkleySockets_Socket_createServer(GScale_Backend *_this,
                                           const int type, const struct sockaddr_storage *saddr)
{
    GScale_Exception except;
    int sockfd = -1;
    int port = 0;
    socklen_t addrlen = 0;

    GScale_Backend_BerkleySockets_Class *instance = NULL;
    GScale_Backend_BerkleySockets_Socket *socket = NULL;

    if(_this==NULL || _this->backend_private==NULL || saddr==NULL) ThrowInvalidArgumentException();

    instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;

    GScale_Backend_BerkleySockets_Socket_create(instance, saddr->ss_family, type, &sockfd);

    switch(saddr->ss_family){
        case AF_INET:
            port = ntohs(((struct sockaddr_in *)saddr)->sin_port);
            addrlen = sizeof(struct sockaddr_in);
            break;
        case AF_INET6:
            port = ntohs(((struct sockaddr_in6 *)saddr)->sin6_port);
            addrlen = sizeof(struct sockaddr_in6);
            break;
    }

    if(bind(sockfd, (const struct sockaddr*)saddr, addrlen)==-1){
        close(sockfd);
        sockfd = -1;
        ThrowException("couldnt bind socket", errno);
    }

    if((type==SOCK_STREAM || type==SOCK_SEQPACKET) && listen(sockfd, 500)==-1){
        if(errno != EOPNOTSUPP){
            close(sockfd);
            sockfd = -1;
            ThrowException("listen call failed", errno);
        }
    }

    Try{
    	/*GSCALE_DEBUGP("Listening on: %s:%d", GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr(saddr), GScale_Backend_BerkleySockets_Util_GetPortFromSockaddr(saddr));*/
        socket = GScale_Backend_BerkleySockets_Socket_construct(_this, sockfd, GSCALE_YES, type, saddr->ss_family, BERKLEYSOCKETS_STATE_ACCEPTING, port);
    }
    Catch(except){
        if(socket!=NULL){
            GScale_Backend_BerkleySockets_Socket_destruct(_this, socket);
        }
        else{
            close(sockfd);
            sockfd = -1;
            /*socket = NULL;*/
        }
        Throw(except);
    }
    return socket;
}

GScale_Backend_BerkleySockets_Socket*
GScale_Backend_BerkleySockets_Socket_accept(GScale_Backend *_this,
                                                    GScale_Backend_BerkleySockets_Socket *serversocket)
{
    GScale_Exception except;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(struct sockaddr_storage);
    GScale_Backend_BerkleySockets_Class *instance = NULL;
    int sockfd = -1;
    GScale_Backend_BerkleySockets_Socket *socket = NULL;

    ThrowErrnoOn((sockfd = accept(serversocket->fd, (struct sockaddr *)&addr, &addrlen)) == -1);

    if(_this==NULL || _this->backend_private==NULL || serversocket==NULL){
        close(sockfd);
        ThrowInvalidArgumentException();
    }
    instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;

    Try{
        socket = GScale_Backend_BerkleySockets_Socket_construct(_this, sockfd, GSCALE_NO, SOCK_STREAM, addr.ss_family, BERKLEYSOCKETS_STATE_CONNECTED, htons(((struct sockaddr_in *)&addr)->sin_port));

        socket->sndreader = mrRingBuffer_initReader(&instance->sndmrmeta);
        socket->eventreader = mrRingBuffer_initReader(&instance->eventmrmeta);
    }
    Catch(except){
        if(socket!=NULL){
            GScale_Backend_BerkleySockets_Socket_destruct(_this, socket);
        }
        else{
            close(sockfd);
            sockfd = -1;
            /*socket = NULL;*/
        }
        Throw(except);
    }
    return socket;
}

/*SOCK_STREAM*/
GScale_Backend_BerkleySockets_Socket*
GScale_Backend_BerkleySockets_Socket_createMulticastReceiver(GScale_Backend *_this, const struct sockaddr_storage *saddr)
{
    GScale_Exception except;
    int sockfd = -1;
    int port = 0;
    socklen_t addrlen = 0;

    struct sockaddr *addr = NULL;
    struct sockaddr_in v4bind;
    struct sockaddr_in6 v6bind;

    struct ip_mreq groupv4;
    struct ipv6_mreq groupv6;
    struct ifaddrs *ifaddr, *ifa;
    struct ifreq ifr;

    GScale_Backend_BerkleySockets_Class *instance = NULL;
    GScale_Backend_BerkleySockets_Socket *socket = NULL;

    if(_this==NULL || _this->backend_private==NULL || saddr==NULL) ThrowInvalidArgumentException();

    instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;

    GScale_Backend_BerkleySockets_Socket_create(instance, saddr->ss_family, SOCK_DGRAM, &sockfd);

    switch(saddr->ss_family){
        case AF_INET:
            port = ntohs(((struct sockaddr_in *)saddr)->sin_port);
            addrlen = sizeof(struct sockaddr_in);

            v4bind.sin_family = AF_INET;
            v4bind.sin_port = htons(port);
            v4bind.sin_addr.s_addr = INADDR_ANY;

            addr = (struct sockaddr*)&v4bind;
            break;
        case AF_INET6:
            port = ntohs(((struct sockaddr_in6 *)saddr)->sin6_port);
            addrlen = sizeof(struct sockaddr_in6);

            v6bind.sin6_family = AF_INET6;
            v6bind.sin6_port = htons(port);
            v6bind.sin6_addr = in6addr_any;

            addr = (struct sockaddr*)&v6bind;
            break;
    }
    if(bind(sockfd, (const struct sockaddr*)addr, addrlen)==-1){
        close(sockfd);
        sockfd = -1;
        ThrowException("couldnt bind socket", errno);
    }

    /****************************************************************************************/

    if (getifaddrs(&ifaddr) == -1) {
        close(sockfd);
        sockfd = -1;
        ThrowException("couldnt list interfaces", errno);
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if(saddr->ss_family != ifa->ifa_addr->sa_family){
            continue;
        }
        switch(saddr->ss_family){
            case AF_INET:
                groupv4.imr_multiaddr.s_addr = ((struct sockaddr_in *)saddr)->sin_addr.s_addr;

                groupv4.imr_interface.s_addr = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;

                setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&groupv4, sizeof(groupv4));

                break;
            case AF_INET6:

                memset(ifr.ifr_name, '\0', sizeof(ifr.ifr_name));
                strncpy(ifr.ifr_name, ifa->ifa_name, sizeof(ifr.ifr_name)-1);
                ioctl(sockfd,SIOCGIFINDEX,&ifr);

                groupv6.ipv6mr_multiaddr = ((struct sockaddr_in6 *)saddr)->sin6_addr;
                /* local IPv6 address of interface */
                groupv6.ipv6mr_interface = ifr.ifr_ifindex;

                setsockopt(sockfd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *)&groupv6, sizeof(groupv6));

                break;
        }
    }

    freeifaddrs(ifaddr);
    /****************************************************************************************/

    Try{
        /*GSCALE_DEBUGP("Listening on: %s:%d", GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr(saddr), GScale_Backend_BerkleySockets_Util_GetPortFromSockaddr(saddr));*/
        socket = GScale_Backend_BerkleySockets_Socket_construct(_this, sockfd, GSCALE_YES, SOCK_DGRAM, saddr->ss_family, BERKLEYSOCKETS_STATE_ACCEPTING, port);
    }
    Catch(except){
        if(socket!=NULL){
            GScale_Backend_BerkleySockets_Socket_destruct(_this, socket);
        }
        else{
            close(sockfd);
            sockfd = -1;
            /*socket = NULL;*/
        }
        Throw(except);
    }
    return socket;
}


GScale_Backend_BerkleySockets_Socket*
GScale_Backend_BerkleySockets_Socket_construct(GScale_Backend *_this,
                                              int sockfd, uint8_t server, int type, int family,
                                              uint8_t state, uint16_t port){
	GScale_Backend_BerkleySockets_Socket *socket = NULL;
	struct epoll_event ev = {0, {0}};
    int flags = 0;

    if ((flags = fcntl(sockfd, F_GETFL, 0)) == -1){
        flags = 0;
    }
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	socket = calloc(1, sizeof(GScale_Backend_BerkleySockets_Socket));
	if(socket==NULL){
		ThrowException("allocation failed", errno);
	}

    socket->fd = sockfd;
    socket->flags.server = 0;
    socket->flags.server |= server;
    socket->type  = type;
    socket->family  = family;
    socket->state = state;
    socket->msgoffset = 0;
    socket->port  = port;
    socket->sndreader = NULL;
    socket->eventreader = NULL;
    socket->synctime.tv_sec = 0;
    socket->synctime.tv_usec = 0;

    gettimeofday(&socket->created, NULL);

    ringBuffer_initMetaPointer(&socket->ringbuffer.internalsendbuffer, &socket->ringbuffer.internalsendmetabuffer);
    ringBuffer_initDataPointer(&socket->ringbuffer.internalsendbuffer, sizeof(socket->ringbuffer.internalsendcbuffer), socket->ringbuffer.internalsendcbuffer);
    ringBuffer_reset(&socket->ringbuffer.internalsendbuffer);

    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    socket->flags.enabled |= 1;

    /*GSCALE_DEBUGP("adding Socket: %d", sockfd);*/
    ThrowErrnoOn(epoll_ctl(((GScale_Backend_BerkleySockets_Class*)_this->backend_private)->evsockets, EPOLL_CTL_ADD, sockfd, &ev)==-1);

    GScale_Backend_Berkleysockets_Database_insertSocket(_this->g->dbhandle, socket);

    return socket;
}

void
GScale_Backend_BerkleySockets_Socket_enableEPOLLOUT(GScale_Backend *_this,GScale_Backend_BerkleySockets_Socket *socket){
    struct epoll_event ev = {0, {0}};
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = socket->fd;
    epoll_ctl(((GScale_Backend_BerkleySockets_Class*)_this->backend_private)->evsockets, EPOLL_CTL_MOD, socket->fd, &ev);
}
void
GScale_Backend_BerkleySockets_Socket_disableEPOLLOUT(GScale_Backend *_this,GScale_Backend_BerkleySockets_Socket *socket){
    struct epoll_event ev = {0, {0}};
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = socket->fd;
    epoll_ctl(((GScale_Backend_BerkleySockets_Class*)_this->backend_private)->evsockets, EPOLL_CTL_MOD, socket->fd, &ev);
}

void
GScale_Backend_BerkleySockets_Socket_destruct(GScale_Backend *_this,
                                             GScale_Backend_BerkleySockets_Socket *socket)
{
    int fd = -1;

    if(_this==NULL || _this->g==NULL || socket==NULL) ThrowInvalidArgumentException();

    /*GSCALE_DEBUGP("removing Socket: %d", socket->fd);*/

    fd = socket->fd;
    socket->flags.enabled = 0;
    GScale_Backend_Berkleysockets_Database_deleteSocket(_this->g->dbhandle, socket);
    free(socket);

    close(fd);
}

#endif
