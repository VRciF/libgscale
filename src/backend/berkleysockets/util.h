#ifndef BERKLEYSOCKETS_UTIL_H_
#define BERKLEYSOCKETS_UTIL_H_

#include <sys/socket.h>

#include "types.h"
#include "backend/berkleysockets/types.h"

/* following function from beejs network programming guide (http://beej.us/guide/bgnet/output/html/multipage/inet_ntopman.html)
 * many thanks to Brian "Beej Jorgensen" Hall
 */
char* GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr(const struct sockaddr_storage *sa) /* throw (GScale_Exception) */;
char* GScale_Backend_BerkleySockets_Util_GetIpFromSockaddr_r(const struct sockaddr_storage *sa, char *s, size_t maxlen) /* throw (GScale_Exception) */;
struct sockaddr_storage*
GScale_Backend_BerkleySockets_Util_GetSockaddrFromIp(int af, const char *src, struct sockaddr_storage *dest);

uint16_t GScale_Backend_BerkleySockets_Util_GetPortFromSockaddr(const struct sockaddr_storage *sa) /* throw (GScale_Exception) */;
struct sockaddr_storage* GScale_Backend_BerkleySockets_Util_IfaceToSockaddr(const char *ifname, struct sockaddr_storage *sa) /* throw (GScale_Exception) */;


uint32_t GScale_Backend_BerkleySockets_Util_ProcessMRSendBuffer(GScale_Backend *_this,
		                                                      GScale_Backend_BerkleySockets_Socket *socket,
		                                                      mrRingBufferReader *reader) /* throw (GScale_Exception) */;
uint32_t GScale_Backend_BerkleySockets_Util_ProcessSendBuffer(GScale_Backend *_this,
                                                              GScale_Backend_BerkleySockets_Socket *socket,
                                                              ringBuffer *rd) /* throw (GScale_Exception) */;
uint32_t GScale_Backend_BerkleySockets_Util_ProcessReadBuffer(GScale_Backend *_this, GScale_Backend_BerkleySockets_Socket *socket, char *buffer, uint32_t len);

void GScale_Backend_BerkleySockets_Util_EnqueueNodeAvailable(GScale_Node *node, ringBuffer *rb);
void GScale_Backend_BerkleySockets_Util_EnqueueNodeUnavailble(GScale_Node *node, ringBuffer *rb);

void GScale_Backend_BerkleySockets_Util_SynchronizeSocket(GScale_Backend *_this, GScale_Backend_BerkleySockets_Socket *socket);

void GScale_Backend_BerkleySockets_Util_ShutdownBackend(GScale_Backend *_this) /* throw (GScale_Exception) */;

void GScale_Backend_BerkleySockets_Util_ProcessServers(GScale_Backend *_this);

void GScale_Backend_BerkleySockets_Util_OptionPort(GScale_Backend *_this, GScale_Backend_Berkleysockets_Options_CmdPort *cmd);

#endif
