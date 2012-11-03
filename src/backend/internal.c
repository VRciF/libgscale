/*
 * local.h
 *
 *  Created on: Oct 22, 2010
 */

#ifndef INTERNAL_C_
#define INTERNAL_C_

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include <unistd.h>

#include "util/log.h"
#include "database.h"
#include "debug.h"
#include "backend/internal.h"

int8_t
GScale_Backend_Internal_CompareNodeIdentifier(const GScale_Node_Identifier *n1,
                                              const GScale_Node_Identifier *n2)
{
    int8_t rval = 0;

    rval = GScale_Backend_Internal_CompareGroupIdentifier(&n1->gid, &n2->gid);
    if (rval == 0) {
        rval = memcmp(n1->uuid, n2->uuid, sizeof(n1->uuid));
    }
    return rval;
}

int8_t
GScale_Backend_Internal_CompareGroupIdentifier(const GScale_GroupIdentifier *g1,
                                               const GScale_GroupIdentifier *g2)
{
    return memcmp(g1->name, g2->name, sizeof(g1->name));
}
int8_t
GScale_Backend_Internal_CompareGroupNamespace(const GScale_GroupIdentifier *g1,
                                              const GScale_GroupIdentifier *g2)
{
    int8_t result = 0;

    result = GScale_Backend_Internal_CompareGroupIdentifier(g1, g2);
    if (result == 0) {
        result = memcmp(g1->nspname, g2->nspname, sizeof(g1->nspname));
    }
    return result;
}

int8_t
GScale_Backend_Internal_HostUUIDEqual(const GScale_Host_UUID *h1,
                                      const GScale_Host_UUID *h2)
{
    return (GScale_Backend_Internal_CompareHostUUID(h1,h2) == 0);
}
int8_t
GScale_Backend_Internal_CompareHostUUID(const GScale_Host_UUID *h1,
                                        const GScale_Host_UUID *h2)
{
    return ( memcmp(*h1, *h2, sizeof(GScale_Host_UUID)) );
}

void
GScale_Backend_Internal_CopyHostUUID(const GScale_Host_UUID *src,
                                     GScale_Host_UUID *dest)
{
    memcpy(dest, src, 16);
}

/************************************ NODE ITERATION *********************************************/

GScale_Node*
GScale_Backend_Internal_Nodes_nextByNode(GScale_Group *g, const GScale_Node* node, int remote)
{
    struct timeval nullval = {0,0};
    GScale_Node *next = NULL;
    char where[1024] = {'\0'};

    if(g==NULL) ThrowInvalidArgumentException();

    if(node!=NULL){
        nullval = node->added;
    }

    snprintf(where, sizeof(where)-1, " remote=%d AND (addedsecond>%lu OR (addedsecond>=%lu AND addedmicrosecond>%lu))", remote, nullval.tv_sec,nullval.tv_sec,nullval.tv_usec);
    next = GScale_Database_getNode(g->dbhandle, where, "addedsecond,addedmicrosecond DESC");

    return next;
}

GScale_Node*
GScale_Backend_Internal_Nodes_nextByAddedTime(GScale_Group *g, const struct timeval *tv, int remote)
{
    struct timeval nullval = {0,0};
    char where[1024] = {'\0'};
    GScale_Node *node = NULL;

    if(g==NULL) ThrowInvalidArgumentException();

    if(tv==NULL){ tv = &nullval; }

    snprintf(where+strlen(where), sizeof(where)-1-strlen(where), " remote=%d AND (addedsecond>%lu OR (addedsecond>=%lu AND addedmicrosecond>%lu))", remote, tv->tv_sec,tv->tv_sec,tv->tv_usec);
    node = GScale_Database_getNode(g->dbhandle, where, "addedsecond,addedmicrosecond DESC");

    return node;
}

/************************************ END NODE ITERATION *********************************************/


const GScale_Host_UUID*
GScale_Backend_Internal_GetBinaryHostUUID()
{
    static GScale_Host_UUID hostuuid = { '\0' };
    static uint8_t init = 0;

    if (!init) {
        init = 1;
        UUID_createBinary(hostuuid);
    }
    return (const GScale_Host_UUID*)&hostuuid;
}
GScale_Host_UUID*
GScale_Backend_Internal_GetBinaryHostUUID_r(GScale_Host_UUID *store)
{
    const GScale_Host_UUID* hostuuid = NULL;

    if (store == NULL) ThrowInvalidArgumentException();

    hostuuid = GScale_Backend_Internal_GetBinaryHostUUID();
    memcpy(*store, hostuuid, 16);

    return store;
}

int8_t
GScale_Backend_Internal_ShallSendToHost(uint8_t dsttype,
                                        const GScale_Node_FQIdentifier *fqid,
                                        const GScale_Host_UUID *possibledesthostuuid)
{
    if (fqid == NULL || possibledesthostuuid == NULL) ThrowInvalidArgumentException();

    if ((dsttype & GSCALE_DESTINATION_TYPE_HOST) == GSCALE_DESTINATION_TYPE_HOST) {
        if (memcmp(fqid->hostuuid, possibledesthostuuid, 16) != 0) {
            return GSCALE_NOK;
        }
    }
    return GSCALE_OK;
}

int8_t
GScale_Backend_Internal_ShallSendToGroup(uint8_t dsttype,
                                         const GScale_Node_FQIdentifier *fqid,
                                         GScale_Group *possibledestgroup)
{
    if (fqid == NULL || possibledestgroup == NULL) ThrowInvalidArgumentException();

    if ((dsttype & GSCALE_DESTINATION_TYPE_GROUP) == GSCALE_DESTINATION_TYPE_GROUP) {
        if (GScale_Backend_Internal_CompareGroupIdentifier(&fqid->id.gid, &possibledestgroup->gid) == 0) {
            return GSCALE_OK;
        }
        return GSCALE_NOK;
    }
    else if ((dsttype & GSCALE_DESTINATION_TYPE_GROUPNAMESPACE) == GSCALE_DESTINATION_TYPE_GROUPNAMESPACE) {
        if (GScale_Backend_Internal_CompareGroupNamespace(&fqid->id.gid, &possibledestgroup->gid) == 0) {
            return GSCALE_OK;
        }
        return GSCALE_NOK;
    }
    return GSCALE_OK;
}
int8_t
GScale_Backend_Internal_ShallSendToGroupNamespace(uint8_t dsttype,
                                                  const GScale_Node_FQIdentifier *fqid,
                                                  GScale_Group *possibledestgroup)
{
    if (fqid == NULL || possibledestgroup == NULL) ThrowInvalidArgumentException();

    if (GSCALE_NOK == GScale_Backend_Internal_ShallSendToGroup(dsttype, fqid, possibledestgroup)) {
        return GSCALE_NOK;
    }
    if ((dsttype & GSCALE_DESTINATION_TYPE_GROUPNAMESPACE) == GSCALE_DESTINATION_TYPE_GROUPNAMESPACE) {
        if (GScale_Backend_Internal_CompareGroupNamespace(&fqid->id.gid, &possibledestgroup->gid) == 0) {
            return GSCALE_OK;
        }
        return GSCALE_NOK;
    }
    return GSCALE_OK;
}
int8_t
GScale_Backend_Internal_ShallSendToNode(uint8_t dsttype,
                                        const GScale_Node_FQIdentifier *fqid,
                                        GScale_Node *possibledestnode)
{
    if (fqid == NULL || possibledestnode == NULL) ThrowInvalidArgumentException();

    if (GSCALE_NOK == GScale_Backend_Internal_ShallSendToGroupNamespace(dsttype, fqid, possibledestnode->locality.local.g)) {
        return GSCALE_NOK;
    }
    if ((dsttype & GSCALE_DESTINATION_TYPE_NODE) == GSCALE_DESTINATION_TYPE_NODE) {
        if (memcmp(fqid->id.uuid, possibledestnode->locality.local.id.uuid, 16) == 0) {
            return GSCALE_OK;
        }
        return GSCALE_NOK;
    }
    return GSCALE_OK;
}

void
GScale_Backend_Internal_OnNodeAvailable(GScale_Backend *_this,
                                        const GScale_Node_FQIdentifier *fqid,
                                        void *tag)
{
    uint8_t isnodelocal = 0;
    GScale_Node* iternode = NULL;

    GScale_Node emptynode = GSCALE_INIT_NODE;
    GScale_Node *rnode = NULL;
    GScale_Exception cexcept;

    if (_this == NULL || fqid == NULL) ThrowInvalidArgumentException();

    isnodelocal = GScale_Backend_Internal_HostUUIDEqual(GScale_Backend_Internal_GetBinaryHostUUID(), &fqid->hostuuid);
    if(!isnodelocal){
        /* if node is empty -> return */
        if(GScale_Backend_Internal_CompareNodeIdentifier(&emptynode.locality.local.id, &fqid->id)==0){
            return;
        }
        /* if the node is already registered -> return*/
        if(GScale_Database_getNodeByUUID(_this->g, NULL, 1, NULL, NULL, &fqid->id.uuid, &fqid->hostuuid)!=NULL){
            /* already available */
            return;
        }

        rnode = (GScale_Node*)calloc(1, sizeof(GScale_Node));
        if(rnode==NULL){
            return;
        }

        rnode->added.tv_sec = 0;
        rnode->added.tv_usec = 0;
        rnode->flags.remote |= 1;
        rnode->backend = _this;
        rnode->locality.remote.tag = tag;
        memcpy(&rnode->locality.remote.id, fqid, sizeof(GScale_Node_FQIdentifier));
        Try{
            GScale_Database_insertNode(_this->g->dbhandle, rnode);
        }Catch(cexcept){
            free(rnode);
            rnode=NULL;
            Throw cexcept;
        }
    }
    /* seems node has connected
     * so we notify the local nodes
     */
    iternode = NULL;
    while ((iternode=GScale_Backend_Internal_Nodes_nextByNode(_this->g, iternode, GSCALE_NO))!=NULL) {
        if (isnodelocal && GScale_Backend_Internal_CompareNodeIdentifier(&iternode->locality.local.id, &fqid->id) == 0) {
            continue;
        }

        if (iternode->locality.local.callbacks.OnNodeAvailable != NULL) {
            iternode->locality.local.callbacks.OnNodeAvailable(iternode, fqid);
        }
    }
}
void
GScale_Backend_Internal_OnNodeUnavailable(GScale_Backend *_this,
                                          const GScale_Node_FQIdentifier *fqid)
{
    uint8_t isnodelocal = 0;
    GScale_Node *iternode = NULL;
    GScale_Node *fqnid = NULL;
    GScale_Node emptynode = GSCALE_INIT_NODE;

    if (_this == NULL || fqid == NULL) ThrowInvalidArgumentException();

    isnodelocal = GScale_Backend_Internal_HostUUIDEqual(
            GScale_Backend_Internal_GetBinaryHostUUID(), &fqid->hostuuid);

    if(!isnodelocal){
        /* if node is empty -> return */
        if(GScale_Backend_Internal_CompareNodeIdentifier(&emptynode.locality.local.id, &fqid->id)==0){
            return;
        }

        if((fqnid=GScale_Database_getNodeByUUID(_this->g, NULL, 1, NULL, NULL, &fqid->id.uuid, &fqid->hostuuid))!=NULL){
            /* already available */
            GScale_Database_deleteNode(_this->g->dbhandle, fqnid);
            free(fqnid);
        }
    }

    /* seems node has disconnected
     * so we notify the local nodes
     */
    iternode = NULL;
    while ((iternode=GScale_Backend_Internal_Nodes_nextByNode(_this->g, iternode, GSCALE_NO))!=NULL) {
        if (isnodelocal && GScale_Backend_Internal_CompareNodeIdentifier(&iternode->locality.local.id, &fqid->id) == 0) {
            continue;
        }

        if (iternode->locality.local.callbacks.OnNodeUnavailable != NULL) {
            iternode->locality.local.callbacks.OnNodeUnavailable(iternode, fqid);
        }

    }
}

int32_t
GScale_Backend_Internal_SendLocalMessage(GScale_Backend *_this,
                                         const GScale_Packet *packet)
{
    GScale_Node* iternode = NULL;

    if (_this == NULL || packet == NULL) ThrowInvalidArgumentException();
    if (GSCALE_NOK == GScale_Backend_Internal_ShallSendToHost(packet->header.dstType, &packet->header.dst, GScale_Backend_Internal_GetBinaryHostUUID())) {
        return 0;
    }
    /* here its sure that destination is our host */
    /* its an error if the src equals its destination! */
    if (GScale_Backend_Internal_CompareNodeIdentifier(&packet->header.src.id, &packet->header.dst.id) == 0) {
        ThrowInvalidArgumentException();
    }

    iternode = NULL;
    while ((iternode=GScale_Backend_Internal_Nodes_nextByNode(_this->g, iternode, GSCALE_NO))!=NULL) {

        if (GScale_Backend_Internal_CompareNodeIdentifier(&iternode->locality.local.id, &packet->header.src.id) == 0) {
            continue; /* dont send the message to the source */
        }

        if (GSCALE_NOK == GScale_Backend_Internal_ShallSendToNode(packet->header.dstType, &packet->header.dst, iternode)) {
            continue;
        }

        if (iternode->locality.local.callbacks.OnRead != NULL) {
            iternode->locality.local.callbacks.OnRead(iternode, packet);
        }

    }

    return packet->data.length;
}

void
GScale_Backend_Internal_SetFQNodeIdentifierByNode(const GScale_Node *node,
                                                  GScale_Node_FQIdentifier *fqid)
{
    if (node == NULL || fqid == NULL) ThrowInvalidArgumentException();

    GScale_Backend_Internal_GetBinaryHostUUID_r(&fqid->hostuuid);
    if(node->flags.remote){
        fqid->id = node->locality.remote.id.id;
    }
    else{
        fqid->id = node->locality.local.id;
    }
}

uint32_t
GScale_Backend_Internal_CountLocalNodes(const GScale_Group *g)
{
    if (g == NULL) ThrowInvalidArgumentException();

    return GScale_Database_CountTable(g->dbhandle, "nodes", "remote=0");
}

void GScale_Backend_Internal_AddEventDescriptor(GScale_Group *g,int evfd, int flags){
    struct epoll_event ev;

    if(g==NULL || evfd<0)ThrowInvalidArgumentException();

    if(flags==0){ flags = EPOLLIN; }
    ev.events = flags;
    ev.data.fd = evfd;
    ThrowErrnoOn(epoll_ctl(g->eventdescriptor[0], EPOLL_CTL_ADD, evfd, &ev) == -1);
}
void GScale_Backend_Internal_RemoveEventDescriptor(GScale_Group *g,int evfd){
    if(g==NULL || evfd<0) ThrowInvalidArgumentException();

    ThrowErrnoOn(epoll_ctl(g->eventdescriptor[0], EPOLL_CTL_DEL, evfd, NULL) == -1);
}
void GScale_Backend_Internal_EnqueueWorker(GScale_Group *g){
    if(g!=NULL){
        write(g->pipefds[1], "1", 1);
    }
}

#endif /* LOCAL_H_ */
