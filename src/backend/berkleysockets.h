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

#ifndef BERKLEYSOCKETS_H_
#define BERKLEYSOCKETS_H_

#include <stdint.h>
#include <sys/time.h>

#include "types.h"

/*********************************** helper functions **********************************************/

void GScale_Backend_BerkleySockets_handleSockets(GScale_Backend *_this, struct timeval *timeout);
void GScale_Backend_BerkleySockets_handleEpoll(GScale_Backend *_this, struct timeval *timeout, int evsocket);

/*********************************** implementation functions **********************************************/

void GScale_Backend_BerkleySockets_GetOption(GScale_Backend *_this, uint16_t optname,
        void *optval, uint16_t optlen);
void GScale_Backend_BerkleySockets_SetOption(GScale_Backend *_this, uint16_t optname, const void *optval, uint16_t optlen);

void GScale_Backend_BerkleySockets_Worker(GScale_Backend *_this, struct timeval *timeout);

/* called when a local node becomes available */
void GScale_Backend_BerkleySockets_OnNodeLocalAvailable(GScale_Backend *_this,
        GScale_Node *node);
/* called when a local node becomes unavailable */
void GScale_Backend_BerkleySockets_OnNodeLocalUnavailable(GScale_Backend *_this,
        GScale_Node *node);
/* called when a local node writes data to the group */
int32_t GScale_Backend_BerkleySockets_OnLocalNodeWritesToGroup(GScale_Backend *_this,
        GScale_Packet *packet);

/* backend init function */
void GScale_Backend_BerkleySockets_Init(GScale_Backend *_this);

#define GScale_Backend_BerkleySockets &GScale_Backend_BerkleySockets_Init

#endif /* BERKLEYSOCKETS_H_ */
