/*
 * localloopback.h
 *
 *  Created on: Oct 21, 2010
 */

#ifndef LOCALLOOPBACK_C_
#define LOCALLOOPBACK_C_

#include <stddef.h>
#include <errno.h>
#include <string.h>

#include "types.h"
#include "util/log.h"
#include "backend/internal.h"
#include "backend/loopback.h"

void
GScale_Backend_LocalLoopback_OnLocalNodeAvailable(GScale_Backend *_this,
                                             GScale_Node *node)
{
    GScale_Node_FQIdentifier fqid;

    GScale_Backend_Internal_SetFQNodeIdentifierByNode(node, &fqid);
    GScale_Backend_Internal_OnNodeAvailable(_this, &fqid, NULL);
}

void
GScale_Backend_LocalLoopback_OnLocalNodeUnavailable(GScale_Backend *_this,
                                               GScale_Node *node)
{
    GScale_Node_FQIdentifier fqid;

    GScale_Backend_Internal_SetFQNodeIdentifierByNode(node, &fqid);
    GScale_Backend_Internal_OnNodeUnavailable(_this, &fqid);
}

int32_t
GScale_Backend_LocalLoopback_OnLocalNodeWritesToGroup(GScale_Backend *_this, GScale_Packet *packet)
{
    return GScale_Backend_Internal_SendLocalMessage(_this, packet);
}

/* DoubleLinkedList remotenodes; */

void
GScale_Backend_LocalLoopback_Init(GScale_Backend *_this)
{
    if (_this == NULL) ThrowInvalidArgumentException();

    _this->backend_private = NULL;

    _this->ioc.OnLocalNodeAvailable = &GScale_Backend_LocalLoopback_OnLocalNodeAvailable;
    _this->ioc.OnLocalNodeUnavailable
            = &GScale_Backend_LocalLoopback_OnLocalNodeUnavailable;

    _this->ioc.OnLocalNodeWritesToGroup
            = &GScale_Backend_LocalLoopback_OnLocalNodeWritesToGroup;

    _this->instance.Worker = NULL;
    _this->instance.GetOption = NULL;
    _this->instance.SetOption = NULL;
}

#endif /* LOCALLOOPBACK_H_ */
