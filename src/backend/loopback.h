/*
 * localloopback.h
 *
 *  Created on: Oct 21, 2010
 */

#ifndef LOCALLOOPBACK_H_
#define LOCALLOOPBACK_H_

#include <stdint.h>
#include "types.h"

/* called when a node becomes available */
void GScale_Backend_LocalLoopback_OnLocalNodeAvailable(GScale_Backend *_this,
        GScale_Node *node);
/* called when a local node becomes unavailable */
void GScale_Backend_LocalLoopback_OnLocalNodeUnavailable(GScale_Backend *_this,
        GScale_Node *node);
/* called when a local node writes data to the group */
int32_t GScale_Backend_LocalLoopback_OnLocalNodeWritesToGroup(GScale_Backend *_this,
        GScale_Packet *packet);

/* backend init function */
void GScale_Backend_LocalLoopback_Init(GScale_Backend *_this);

#define GScale_Backend_LocalLoopback &GScale_Backend_LocalLoopback_Init

#endif /* LOCALLOOPBACK_H_ */
