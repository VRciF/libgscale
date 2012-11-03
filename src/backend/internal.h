/*
 * local.h
 *
 *  Created on: Oct 22, 2010
 */

#ifndef INTERNAL_H_
#define INTERNAL_H_

#include <stdint.h>
#include "types.h"

int8_t GScale_Backend_Internal_CompareNodeIdentifier(
        const GScale_Node_Identifier *n1, const GScale_Node_Identifier *n2);

int8_t GScale_Backend_Internal_CompareGroupIdentifier(
        const GScale_GroupIdentifier *g1, const GScale_GroupIdentifier *g2);
int8_t GScale_Backend_Internal_CompareGroupNamespace(
        const GScale_GroupIdentifier *g1, const GScale_GroupIdentifier *g2);

int8_t GScale_Backend_Internal_HostUUIDEqual(const GScale_Host_UUID *h1,
        const GScale_Host_UUID *h2);
int8_t GScale_Backend_Internal_CompareHostUUID(const GScale_Host_UUID *h1,
        const GScale_Host_UUID *h2);
void GScale_Backend_Internal_CopyHostUUID(const GScale_Host_UUID *src,
        GScale_Host_UUID *dest);

const GScale_Host_UUID* GScale_Backend_Internal_GetBinaryHostUUID();
GScale_Host_UUID* GScale_Backend_Internal_GetBinaryHostUUID_r(
        GScale_Host_UUID *store);

int8_t GScale_Backend_Internal_ShallSendToHost(const uint8_t dsttype,
        const GScale_Node_FQIdentifier *fqid,
        const GScale_Host_UUID *possibledesthostuuid);
int8_t GScale_Backend_Internal_ShallSendToGroup(const uint8_t dsttype,
        const GScale_Node_FQIdentifier *fqid, GScale_Group *g);
int8_t GScale_Backend_Internal_ShallSendToGroupNamespace(const uint8_t dsttype,
        const GScale_Node_FQIdentifier *fqid, GScale_Group *g);
int8_t GScale_Backend_Internal_ShallSendToNode(const uint8_t dsttype,
        const GScale_Node_FQIdentifier *fqid, GScale_Node *possibledestnode);

void GScale_Backend_Internal_OnNodeAvailable(GScale_Backend *_this,
        const GScale_Node_FQIdentifier *fqid, void *tag);
void GScale_Backend_Internal_OnNodeUnavailable(GScale_Backend *_this,
        const GScale_Node_FQIdentifier *fqid);
int32_t GScale_Backend_Internal_SendLocalMessage(GScale_Backend *_this,
        const GScale_Packet *packet);

GScale_Node* GScale_Backend_Internal_Nodes_nextByNode(GScale_Group *g, const GScale_Node* node, int remote);
GScale_Node* GScale_Backend_Internal_Nodes_nextByAddedTime(GScale_Group *g, const struct timeval *tv, int remote);
GScale_Node*
GScale_Backend_Internal_Nodes_getNodeByUUID(const GScale_Group *g, const GScale_Node *node, const int remote,
                                            const GScale_Group_Namespace *nspname, const GScale_Group_Name *name, const GScale_Node_UUID *nodeuuid, const GScale_Host_UUID *hostuuid);

void GScale_Backend_Internal_SetFQNodeIdentifierByNode(
        const GScale_Node *node, GScale_Node_FQIdentifier *fqid);
uint32_t GScale_Backend_Internal_CountLocalNodes(const GScale_Group *g);

void GScale_Backend_DebugLog_NodePacket(GScale_Packet *np);
void GScale_Backend_DebugLog_NodePacketHeader(
                GScale_Packet_Header *header);
void GScale_Backend_DebugLog_NodeFQIdentifier(GScale_Node_FQIdentifier *fqid);
void GScale_Backend_DebugLog_HostUUID(GScale_Host_UUID *huuid);
void GScale_Backend_DebugLog_NodeUUID(GScale_Node_UUID *nuuid);
void GScale_Backend_DebugLog_NodeIdentifier(GScale_Node_Identifier *nid);
void GScale_Backend_DebugLog_GroupIdentifier(GScale_GroupIdentifier *gid);

void GScale_Backend_Internal_AddEventDescriptor(GScale_Group *g,int evfd, int flags);
void GScale_Backend_Internal_RemoveEventDescriptor(GScale_Group *g,int evfd);
void GScale_Backend_Internal_EnqueueWorker(GScale_Group *g);

#endif /* LOCAL_H_ */
