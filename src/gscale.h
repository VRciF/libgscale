/*
 * gscale.h
 *
 *  Created on: Oct 16, 2010
 */

#ifndef GSCALE_H_
#define GSCALE_H_

#include <sys/time.h>

#include "error.h"
#include "types.h"
#include "util/log.h"

int8_t GScale_Group_setNamespace(GScale_Group *g, char *name, uint8_t length);
int8_t GScale_Group_setGroupname(GScale_Group *g, char *name, uint8_t length);

int8_t GScale_Group_init(GScale_Group *g);
int8_t GScale_Group_initBackend(GScale_Group *g,
        GScale_Backend_Init initcallback);

int8_t GScale_Group_shutdown(GScale_Group *g);
int8_t GScale_Group_shutdownBackend(GScale_Group *g,
        GScale_Backend_Init initcallback);

int8_t GScale_Group_Backend_setOption(GScale_Group *g,
        GScale_Backend_Init initcallback, uint16_t optname, const void *optval,
        uint16_t optlen);
int8_t GScale_Group_Backend_getOption(GScale_Group *g,
        GScale_Backend_Init initcallback, uint16_t optname, void *optval,
        uint16_t optlen);
int8_t GScale_Group_Backend_setNodeOption(GScale_Group *g,
        GScale_Backend_Init initcallback, uint16_t optname, const GScale_Node *node, const void *optval,
        uint16_t optlen);
int8_t GScale_Group_Backend_getNodeOption(GScale_Group *g,
        GScale_Backend_Init initcallback, uint16_t optname, const GScale_Node *node, void *optval,
        uint16_t optlen);

int8_t GScale_Group_set(GScale_Group *g, GScale_Group_Setting optname, const void *optval, uint16_t optlen);
int8_t GScale_Group_get(GScale_Group *g, GScale_Group_Setting optname, void *optval, uint16_t optlen);
void GScale_Group_Worker(GScale_Group *g, struct timeval *timeout);

GScale_Node* GScale_Group_connect(GScale_Group *g, GScale_NodeCallbacks callbacks, GScale_Node_Alias *alias);
void GScale_Group_disconnect(GScale_Node *node);

int32_t GScale_Group_write(uint8_t type, GScale_Node *src,
        const GScale_Node_FQIdentifier *dst, GScale_Packet_Payload *data);
int32_t GScale_Group_writeBackend(GScale_Backend_Init initcallback, uint8_t type, GScale_Node *src,
        const GScale_Node_FQIdentifier *dst, GScale_Packet_Payload *data);

int GScale_Group_getEventDescriptor(GScale_Group *g);

#endif /* GSCALE_H_ */
