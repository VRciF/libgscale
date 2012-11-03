#ifndef BERKLEYSOCKETS_UDP_C
#define BERKLEYSOCKETS_UDP_C

#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <time.h>

#include "util/log.h"
#include "backend/internal.h"
#include "backend/berkleysockets/types.h"
#include "backend/berkleysockets/udp.h"
#include "backend/berkleysockets/util.h"
#include "backend/berkleysockets/database.h"

void
GScale_Backend_BerkleySockets_UDP_Broadcast(GScale_Backend *_this)
{
    GScale_Backend_BerkleySockets_Class *instance = NULL;
    struct timeval now = {0, 0};
    static GScale_Backend_BerkleySockets_BroadcastPacket packet;
    uint32_t *uiptr = NULL;
    GScale_Backend_BerkleySockets_Socket *socket = NULL;
    struct ifaddrs *ifaddr=NULL, *ifa = NULL;
    struct sockaddr *destaddr = NULL;
    struct sockaddr_in destaddr4; /* connector's address information for ipv4 */
    struct sockaddr_in6 destaddr6; /* connector's address information for ipv6 */
    char ip[INET6_ADDRSTRLEN+1];
    uint8_t caught = GSCALE_NO;
    uint16_t dstport = 0;
    GScale_Exception except;
    struct sockaddr_storage daddr;

    if(_this==NULL) ThrowInvalidArgumentException();

    instance = (GScale_Backend_BerkleySockets_Class*) _this->backend_private;
    if (instance == NULL) ThrowInvalidArgumentException();

    gettimeofday(&now, NULL);

    /* fill broadcast packet */
    packet.majorversion = GSCALE_VERSION_MAJOR;
    packet.minorversion = GSCALE_VERSION_MINOR;

    packet.t_sec = now.tv_sec;
    packet.t_usec = now.tv_usec;
    packet.nodecount = htonl(GScale_Backend_Internal_CountLocalNodes(_this->g));

    uiptr = (uint32_t*)&packet.t_sec;
    uiptr[0] = htonl(uiptr[0]);
    uiptr[1] = htonl(uiptr[1]);
    uiptr = (uint32_t*)&packet.t_usec;
    uiptr[0] = htonl(uiptr[0]);
    uiptr[1] = htonl(uiptr[1]);

    GScale_Backend_Internal_GetBinaryHostUUID_r(&packet.srchostuuid);
    packet.gid = _this->g->gid;

    if(getifaddrs(&ifaddr)==-1){
        ifaddr = NULL;
    }
    Try{
            socket = NULL;
            while((socket=GScale_Backend_Berkleysockets_Database_getSocketBySettings(_this->g->dbhandle, socket, -1, SOCK_STREAM, -1, 1, NULL))!=NULL){
                packet.srcport = htons(socket->port);

                ((struct sockaddr_in*)&daddr)->sin_family = AF_INET;
                ((struct sockaddr_in*)&daddr)->sin_port = htons(8000);
                inet_pton(AF_INET, "239.255.92.19", &(((struct sockaddr_in*)&daddr)->sin_addr));
                sendto(instance->v4_dgram_socket, &packet, sizeof(packet), 0, (struct sockaddr*)&daddr, sizeof(daddr));

                ((struct sockaddr_in6*)&daddr)->sin6_family = AF_INET6;
                ((struct sockaddr_in6*)&daddr)->sin6_port = htons(8000);
                inet_pton(AF_INET6, "ff00:0:0:0:0:0:efff:5c13", &(((struct sockaddr_in6*)&daddr)->sin6_addr));
                sendto(instance->v6_dgram_socket, &packet, sizeof(packet), 0, (struct sockaddr*)&daddr, sizeof(daddr));

                continue;
                ((struct sockaddr_in*)&daddr)->sin_family = AF_INET;
                ((struct sockaddr_in*)&daddr)->sin_port = htons(8000);
                inet_pton(AF_INET, "255.255.255.255", &(((struct sockaddr_in*)&daddr)->sin_addr));
                sendto(instance->v4_dgram_socket, &packet, sizeof(packet), 0, (struct sockaddr*)&daddr, sizeof(daddr));
                ((struct sockaddr_in*)&daddr)->sin_family = AF_INET;
                ((struct sockaddr_in*)&daddr)->sin_port = htons(8000);
                inet_pton(AF_INET, "127.255.255.255", &(((struct sockaddr_in*)&daddr)->sin_addr));
                sendto(instance->v4_dgram_socket, &packet, sizeof(packet), 0, (struct sockaddr*)&daddr, sizeof(daddr));

                continue;

                if(ifaddr!=NULL){
                    /* Walk through linked list, maintaining head pointer so we
                       can free list later */
                    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                        if (ifa->ifa_addr == NULL || ifa->ifa_ifu.ifu_broadaddr==NULL || ifa->ifa_ifu.ifu_dstaddr){
                            continue;
                        }

                        switch(ifa->ifa_addr->sa_family){
                            case AF_INET:
                                destaddr4 = *(struct sockaddr_in*)ifa->ifa_ifu.ifu_broadaddr;
                                destaddr4.sin_family = ifa->ifa_addr->sa_family;
                                destaddr4.sin_port = (in_port_t)htons(dstport);
                                destaddr = (struct sockaddr*) &destaddr4;
                                break;
                            case AF_INET6:
                                destaddr6 = *(struct sockaddr_in6*)ifa->ifa_ifu.ifu_dstaddr;
                                destaddr6.sin6_family = ifa->ifa_addr->sa_family;
                                destaddr6.sin6_port = (in_port_t)htons(dstport);
                                destaddr = (struct sockaddr*) &destaddr6;
                                break;
                            default:
                                continue;
                        }

                        GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr_r((const struct sockaddr_storage *)&destaddr, ip, sizeof(ip));

                        /*GSCALE_DEBUGP("Broadcasting to: %s:%u",ip, GScale_Backend_BerkleySockets_Util_GetPortFromSockaddr((const struct sockaddr_storage *)&destaddr));*/
                        sendto(instance->v4_dgram_socket, &packet, sizeof(packet), 0, destaddr, sizeof(*destaddr));
                    }
                }
        }
    }
    Catch(except){ caught = GSCALE_YES; }

    /* clean up */
    if(ifaddr!=NULL){
        freeifaddrs(ifaddr);
    }

    if(caught){
        Throw(except);
    }

    instance->lastbroadcast = time(NULL);
}

/**
 * addr can be struct in_addr* OR struct in6_addr* in network byte order, see inet_pton()
 */
uint8_t GScale_Backend_BerkleySockets_UDP_isMulticast(int af, void *addr){
    switch(af){
        case AF_INET:
            if(( ((uint8_t*)&((struct in_addr*)addr)->s_addr)[0] & 0xE0) == 0xE0)
                return GSCALE_YES;
            break;
        case AF_INET6:
            if((((struct in6_addr*)addr)->s6_addr[0] & 0xFF) == 0xFF)
                return GSCALE_YES;
            break;
    }
    return GSCALE_NO;
}


#endif
