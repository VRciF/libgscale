#ifndef BERKLEYSOCKETS_SOCKETOPS_H_
#define BERKLEYSOCKETS_SOCKETOPS_H_

#include <sys/socket.h>

#include "backend/berkleysockets/types.h"
#include "types.h"

void GScale_Backend_BerkleySockets_Socket_create(GScale_Backend_BerkleySockets_Class *instance, const int domain, const int type, int *sockfd);

/*SOCK_STREAM*/
GScale_Backend_BerkleySockets_Socket* GScale_Backend_BerkleySockets_Socket_createServer(GScale_Backend *_this, const int type, const struct sockaddr_storage *saddr);
GScale_Backend_BerkleySockets_Socket* GScale_Backend_BerkleySockets_Socket_connect(GScale_Backend *_this, struct sockaddr_storage *addr);
GScale_Backend_BerkleySockets_Socket* GScale_Backend_BerkleySockets_Socket_accept(GScale_Backend *_this, GScale_Backend_BerkleySockets_Socket *serversocket);
GScale_Backend_BerkleySockets_Socket*
GScale_Backend_BerkleySockets_Socket_createMulticastReceiver(GScale_Backend *_this, const struct sockaddr_storage *saddr);

GScale_Backend_BerkleySockets_Socket*
GScale_Backend_BerkleySockets_Socket_construct(GScale_Backend *_this,
                                              int sockfd, uint8_t server, int type, int family,
                                              uint8_t state, uint16_t port);
void GScale_Backend_BerkleySockets_Socket_enableEPOLLOUT(GScale_Backend *_this,GScale_Backend_BerkleySockets_Socket *socket);
void GScale_Backend_BerkleySockets_Socket_disableEPOLLOUT(GScale_Backend *_this,GScale_Backend_BerkleySockets_Socket *socket);
void GScale_Backend_BerkleySockets_Socket_destruct(GScale_Backend *_this, GScale_Backend_BerkleySockets_Socket *socket);

#endif
