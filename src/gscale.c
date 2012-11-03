/*
 * gscale.h
 *
 *  Created on: Oct 16, 2010
 */

#ifndef GSCALE_C_
#define GSCALE_C_

#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "gscale.h"

#include "util/log.h"

#include "types.h"
#include "util/uuid.h"
#include "backend/internal.h"

#include "exception.h"
#include "database.h"

int8_t GScale_Group_setNamespace(GScale_Group *g, char *name, uint8_t length) {
    if (length > sizeof(g->gid.nspname)) {
        errno = EINVAL;
        return GSCALE_NOK;
    }

    memset(g->gid.nspname, '\0', sizeof(g->gid.nspname));
    memcpy(g->gid.nspname, name, length);

    return GSCALE_OK;
}
int8_t GScale_Group_setGroupname(GScale_Group *g, char *name, uint8_t length) {
    if (length > sizeof(g->gid.name)) {
        errno = EINVAL;
        return GSCALE_NOK;
    }

    memset(g->gid.name, '\0', sizeof(g->gid.name));
    memcpy(g->gid.name, name, length);

    return GSCALE_OK;
}

int8_t GScale_Group_init(GScale_Group *g) {
    GScale_Exception except;

    Try {
        if (g == NULL) {
            ThrowException("invalid argument given", EINVAL);
        }

        memset(g, '\0', sizeof(GScale_Group));
        g->eventdescriptor[0] = g->eventdescriptor[1] = -1;
        g->pipefds[0] = g->pipefds[1] = -1;

        g->eventdescriptor[0] = epoll_create(128);
        if(g->eventdescriptor[0]==-1){
            ThrowException("epoll_create failed", errno);
        }
        if(pipe(g->pipefds)==-1){
            ThrowException("pipe failed", errno);
        }
        if(fcntl(g->pipefds[0], F_SETFL, fcntl(g->pipefds[0], F_GETFL, 0) | O_NONBLOCK)==-1){
            ThrowException("fcntl failed", errno);
        }
        if(fcntl(g->pipefds[1], F_SETFL, fcntl(g->pipefds[1], F_GETFL, 0) | O_NONBLOCK)==-1){
            ThrowException("fcntl failed", errno);
        }

        GScale_Backend_Internal_AddEventDescriptor(g, g->pipefds[0], EPOLLIN);

        GScale_Database_Init(&g->dbhandle);

        DoubleLinkedList_init((DoubleLinkedList*) &g->backends);
    }
    Catch (except) {
        if(except.code!=0){
            errno = except.code;
        }
        GSCALE_ERR(except.message);

        if(g!=NULL){
            if(g->eventdescriptor[0]!=-1){ close(g->eventdescriptor[0]); }
            if(g->pipefds[0]!=-1){ close(g->pipefds[0]); }
            if(g->pipefds[1]!=-1){ close(g->pipefds[1]); }

            Try{
                GScale_Database_Shutdown(g->dbhandle);
                DoubleLinkedList_freeNodes(&g->backends);
            }
            Catch_anonymous{}
        }

        return GSCALE_NOK;
    }

    return GSCALE_OK;
}

int8_t GScale_Group_initBackend(GScale_Group *g,
        GScale_Backend_Init initcallback)
{
    GScale_Backend *_this = NULL;

    if (g == NULL || initcallback == NULL) {
        GSCALE_ERR("invalid argument given");
        errno = EINVAL;
        return GSCALE_NOK;
    }

    Try{
        _this = (GScale_Backend*)DoubleLinkedList_iterBegin(&g->backends);
        while(DoubleLinkedList_iterIsValid(_this)){
            if(_this->initcallback == initcallback){
                GSCALE_ERR("invalid argument given");
                errno = EALREADY;
                return GSCALE_NOK;
            }
            _this = (GScale_Backend*)DoubleLinkedList_iterNext(_this);
        }

        _this = (GScale_Backend*) DoubleLinkedList_allocNode(
                &g->backends, sizeof(GScale_Backend));
        if (_this != NULL) {
            memset(_this, '\0', sizeof(*_this));

            _this->g = g;
            _this->initcallback = initcallback;
            Try{
                initcallback(_this);
            }
            Catch_anonymous{
                DoubleLinkedList_freeNode(&g->backends, _this);
                return GSCALE_NOK;
            }

            return GSCALE_OK;
        }
    }
    Catch_anonymous{}

    return GSCALE_NOK;
}

int8_t GScale_Group_shutdown(GScale_Group *g){
    GScale_Backend *_this = NULL;

    if (g == NULL) {
        GSCALE_ERR("invalid argument given");
        errno = EINVAL;

        return GSCALE_NOK;
    }

    Try{
        _this = (GScale_Backend*)DoubleLinkedList_iterBegin(&g->backends);
        while(DoubleLinkedList_iterIsValid(_this)){
            GScale_Group_shutdownBackend(g, _this->initcallback);
            _this = (GScale_Backend*)DoubleLinkedList_iterNext(_this);
        }

        /*DoubleLinkedList_freeNodes((DoubleLinkedList*) &g->localnodes);*/
        /*DoubleLinkedList_freeNodes((DoubleLinkedList*) &g->remotenodes);*/
        DoubleLinkedList_freeNodes((DoubleLinkedList*) &g->backends);

        GScale_Database_Shutdown(g->dbhandle);
    }
    Catch_anonymous{
        return GSCALE_NOK;
    }
    return GSCALE_OK;
}
int8_t GScale_Group_shutdownBackend(GScale_Group *g,
        GScale_Backend_Init initcallback)
{
    GScale_Backend *_this = NULL;

    if (g == NULL || initcallback == NULL) {
        GSCALE_ERR("invalid argument given");
        errno = EINVAL;
        return GSCALE_NOK;
    }

    Try{
        _this = (GScale_Backend*)DoubleLinkedList_iterBegin(&g->backends);

        while(DoubleLinkedList_iterIsValid(_this)){
            if(_this->initcallback == initcallback){
                if(_this->instance.Shutdown!=NULL){
                    _this->instance.Shutdown(_this);
                }
                DoubleLinkedList_freeNode(&g->backends, _this);
                return GSCALE_OK;
            }
            _this = (GScale_Backend*)DoubleLinkedList_iterNext(_this);
        }
    }
    Catch_anonymous{}

    GSCALE_ERR("backend not initialized");
    errno = EINVAL;
    return GSCALE_NOK;
}

GScale_Node* GScale_Group_connect(GScale_Group *g, GScale_NodeCallbacks callbacks, GScale_Node_Alias *alias) {
    GScale_Backend *backenditer = NULL;
    GScale_Node *node = NULL;

    if (g == NULL) {
        GSCALE_ERR("invalid argument given");
        errno = EINVAL;
        return NULL;
    }
    node = (GScale_Node*)calloc(1, sizeof(GScale_Node));
    if (node == NULL) {
        GSCALE_ERR("invalid argument given");
        errno = EINVAL;
        return NULL;
    }

    Try{
        memset(node, '\0', sizeof(node));
        /*node = (GScale_Node*) DoubleLinkedList_allocNode(
                &g->localnodes, sizeof(GScale_Node));
        */

        node->flags.remote = 0;
        node->locality.local.callbacks = callbacks;
        node->locality.local.g = g;
        node->backend = NULL;
        UUID_createBinary(node->locality.local.id.uuid);
        memcpy(&node->locality.local.id.gid, &g->gid, sizeof(g->gid));
        if(alias!=NULL){
            memcpy(node->locality.local.id.alias, *alias, sizeof(GScale_Node_Alias));
        }
        gettimeofday(&node->added, NULL);
        /* insert node into DB */
        {
            GScale_Database_insertNode(g->dbhandle, node);
        }
        /* now notify backends about the new node */
        for (backenditer = (GScale_Backend*) DoubleLinkedList_iterBegin(&g->backends);
             DoubleLinkedList_iterIsValid(backenditer);
             backenditer = (GScale_Backend*) DoubleLinkedList_iterNext(backenditer)) {
            if (backenditer->ioc.OnLocalNodeAvailable != NULL) {
                backenditer->ioc.OnLocalNodeAvailable(backenditer, node);
            }
        }
    }
    Catch_anonymous{
        free(node);
        node = NULL;
    }
    return node;
}

void GScale_Group_disconnect(GScale_Node *node)
{
    GScale_Backend *backenditer = NULL;

    if (node == NULL) {
        GSCALE_ERR("invalid argument given");
        errno = EINVAL;
        return;
    }

    Try{
        /* now notify backends about the disconnecting */
        for (backenditer = (GScale_Backend*) DoubleLinkedList_iterBegin(&node->locality.local.g->backends);
             DoubleLinkedList_iterIsValid(backenditer);
             backenditer = (GScale_Backend*) DoubleLinkedList_iterNext(backenditer)) {

            if (backenditer->ioc.OnLocalNodeUnavailable != NULL) {
                backenditer->ioc.OnLocalNodeUnavailable(backenditer, node);
            }
        }

        GScale_Database_deleteNode(node->locality.local.g->dbhandle, node);
        free(node);
    }
    Catch_anonymous{}
}

int32_t GScale_Group_write(uint8_t type, GScale_Node *src,
        const GScale_Node_FQIdentifier *dst, GScale_Packet_Payload *data)
{
    return GScale_Group_writeBackend(NULL, type, src, dst, data);
}
int32_t GScale_Group_writeBackend(GScale_Backend_Init initcallback, uint8_t type, GScale_Node *src,
        const GScale_Node_FQIdentifier *dst, GScale_Packet_Payload *data){
    GScale_Packet wpacket;
    uint32_t written = 0;
    GScale_Backend *backenditer = NULL;

    if (src == NULL || data == NULL) {
        GSCALE_ERR("invalid argument given");
        errno = EINVAL;
        return GSCALE_NOK;
    }
    if((type == GSCALE_DESTINATION_TYPE_HOST || type == GSCALE_DESTINATION_TYPE_NODE) && dst==NULL){
        GSCALE_ERR("dst ptr needed for HOST or NODE destination");
        errno = EINVAL;
        return GSCALE_NOK;
    }

    Try{
        GScale_Backend_Internal_SetFQNodeIdentifierByNode(src, &wpacket.header.src);
        if(dst!=NULL){
            wpacket.header.dst = *dst;
        }
        else{
            memset(&wpacket.header.dst, '\0', sizeof(wpacket.header.dst));
            switch(type){
                case GSCALE_DESTINATION_TYPE_GROUPNAMESPACE:
                    memcpy(wpacket.header.dst.id.gid.nspname, src->locality.local.g->gid.nspname, sizeof(src->locality.local.g->gid.nspname));
                    /* break; no break needed! */
                case GSCALE_DESTINATION_TYPE_GROUP:
                    memcpy(wpacket.header.dst.id.gid.name, src->locality.local.g->gid.name, sizeof(src->locality.local.g->gid.name));
                    break;
            }
        }
        wpacket.header.packetid = src->locality.local.g->packetid;
        wpacket.header.dstType = type;
        wpacket.data = *data;

        for (backenditer = (GScale_Backend*) DoubleLinkedList_iterBegin(&src->locality.local.g->backends);
             DoubleLinkedList_iterIsValid(backenditer);
             backenditer = (GScale_Backend*) DoubleLinkedList_iterNext(backenditer)) {

            if (initcallback == NULL || backenditer->initcallback == initcallback) {
                if (backenditer->ioc.OnLocalNodeWritesToGroup != NULL) {
                    written = backenditer->ioc.OnLocalNodeWritesToGroup(backenditer, &wpacket);
                }
                if(initcallback!=NULL){
                    break;
                }
            }
        }
    }
    Catch_anonymous{
        return GSCALE_NOK;
    }

    return written;
}

/*
 * this one searches for a specific backend
 * so it will be O(n) where n is how many backends are registered
 * altough this could be slow e.g. if ure using many backends
 * currently ill accept this issue for better API readability
 * cause ill expect that most use cases need only a few backends
 */
int8_t GScale_Group_Backend_setOption(GScale_Group *g,
        GScale_Backend_Init initcallback, uint16_t optname, const void *optval,
        uint16_t optlen)
{
    GScale_Backend *backenditer = NULL;

    if (g == NULL || initcallback == NULL) {
        GSCALE_ERR("invalid argument given");
        errno = EINVAL;
        return GSCALE_NOK;
    }

    Try{
        for (backenditer = (GScale_Backend*) DoubleLinkedList_iterBegin(&g->backends);
             DoubleLinkedList_iterIsValid(backenditer);
             backenditer = (GScale_Backend*) DoubleLinkedList_iterNext(backenditer)) {

            if (backenditer->initcallback == initcallback) {
                if (backenditer->instance.SetOption != NULL) {
                    backenditer->instance.SetOption(backenditer, optname, optval, optlen);
                    return GSCALE_OK;
                }
            }
        }
    }
    Catch_anonymous{}

    return GSCALE_NOK;

}
int8_t GScale_Group_Backend_getOption(GScale_Group *g,
        GScale_Backend_Init initcallback, uint16_t optname, void *optval,
        uint16_t optlen)
{
    GScale_Backend *backenditer = NULL;

    if (g == NULL || initcallback == NULL) {
        GSCALE_ERR("invalid argument given");
        errno = EINVAL;
        return GSCALE_NOK;
    }

    Try{
        for (backenditer = (GScale_Backend*) DoubleLinkedList_iterBegin(&g->backends);
             DoubleLinkedList_iterIsValid(backenditer);
             backenditer = (GScale_Backend*) DoubleLinkedList_iterNext(backenditer)) {

            if (backenditer->initcallback == initcallback) {
                if (backenditer->instance.GetOption != NULL) {
                    backenditer->instance.GetOption(backenditer, optname, optval, optlen);
                    return GSCALE_OK;
                }
            }
        }
    }
    Catch_anonymous{}

    return GSCALE_NOK;

}

void GScale_Group_Worker(GScale_Group *g, struct timeval *timeout) {
    struct timeval begin,end, diff;
    struct timeval slice, *pslice = NULL;
    GScale_Backend *backenditer = NULL;
    char nullbuff[128] = {'\0'};

    if (g == NULL) {
        GSCALE_ERR("invalid argument given");
        errno = EINVAL;
        return;
    }

    Try{
        errno = 0;
        while(read(g->pipefds[0], nullbuff, sizeof(nullbuff))>0 || errno==EINTR) {}

        timerclear(&begin);
        timerclear(&end);
        timerclear(&diff);

        pslice = &slice;
        timerclear(&slice);
        if(timeout!=NULL && timerisset(timeout)){
            uint32_t cnt  = DoubleLinkedList_countNodes(&g->backends);
            if(cnt>0){
                slice.tv_sec  = timeout->tv_sec/cnt;
                slice.tv_usec = timeout->tv_usec/cnt;
            }
            else{
                slice.tv_sec = 0;
            }
            if(cnt<=0 || !timerisset(&slice)){
                /* in case the division results to an unset timeval i set it to 100 ms*/
                slice.tv_usec = 100000;  /* 100 milliseconds */
            }
        }
        else{
            pslice = timeout;
        }

        for (backenditer = (GScale_Backend*) DoubleLinkedList_iterBegin(&g->backends);
             DoubleLinkedList_iterIsValid(backenditer);
             backenditer = (GScale_Backend*) DoubleLinkedList_iterNext(backenditer)) {

            if (backenditer->instance.Worker != NULL) {
                gettimeofday(&begin, NULL);

                backenditer->instance.Worker(backenditer, pslice);
                gettimeofday(&end, NULL);

                timersub(&end, &begin, &diff);
                if(timeout!=NULL && timerisset(timeout)){
                    timersub(timeout, &diff, timeout);
                }
            }

        }
    }Catch_anonymous{}

    return;
}

int8_t GScale_Group_set(GScale_Group *g, GScale_Group_Setting optname, const void *optval, uint16_t optlen){
	if(g==NULL || optval==NULL){
		GSCALE_ERR("invalid argument given");
		errno = EINVAL;
		return GSCALE_NOK;
	}

	Try{
        switch(optname){
            case ConnectTimeout:
                if(optlen != sizeof(struct timeval)){
                    return GSCALE_NOK;
                }
                g->settings.enable.connecttimeout |= GSCALE_OK;
                g->settings.connecttimeout = *(struct timeval*)optval;
                break;
            case DisconnectTimeout:
                if(optlen != sizeof(struct timeval)){
                    return GSCALE_NOK;
                }
                g->settings.enable.disconnecttimeout |= GSCALE_OK;
                g->settings.disconnecttimeout = *(struct timeval*)optval;
                break;
            case MIN:
            case MAX:
            default:
                break;
        }
	}
	Catch_anonymous{
	    return GSCALE_NOK;
	}

	return GSCALE_OK;
}
int8_t GScale_Group_get(GScale_Group *g, GScale_Group_Setting optname, void *optval, uint16_t optlen){
	if(g==NULL || optval==NULL){
		GSCALE_ERR("invalid argument given");
		errno = EINVAL;
		return GSCALE_NOK;
	}

	Try{
        switch(optname){
            case ConnectTimeout:
                if(optlen != sizeof(struct timeval)){
                    return GSCALE_NOK;
                }
                g->settings.enable.connecttimeout |= GSCALE_OK;
                *(struct timeval*)optval = g->settings.connecttimeout;
                break;
            case DisconnectTimeout:
                if(optlen != sizeof(struct timeval)){
                    return GSCALE_NOK;
                }
                g->settings.enable.disconnecttimeout |= GSCALE_OK;
                *(struct timeval*)optval = g->settings.disconnecttimeout;
                break;
            case MIN:
            case MAX:
            default:
                break;
        }
    }
    Catch_anonymous{
        return GSCALE_NOK;
    }

	return GSCALE_OK;
}

/**
 * this function trie's to always return a valid duplicate of the read side of
 * the internally used event pipe
 * this means:
 * o) use the descriptor for read only! don't close it
 * o) in case the descriptor get's closed during production use,
 *       e.g. cause of a bug or lack of knowledge,
 *    GScale_Group_getEventDescriptor will return a new copy of the descriptor
 *
 * in the end, it means one can assume that this function will always return a valid descriptor
 *    IF the pipe has been initialized correctly by GScale_Init()
 * under any circumstances
 */
int GScale_Group_getEventDescriptor(GScale_Group *g)
{
    struct stat buf;
    if(g==NULL){
        GSCALE_ERR("invalid argument given");
        errno = EINVAL;
        return GSCALE_NOK;
    }

    if(fstat(g->eventdescriptor[1], &buf)==-1 || S_ISREG(buf.st_mode)){
        g->eventdescriptor[1] = dup(g->eventdescriptor[0]);
    }

    return g->eventdescriptor[1];
}


#endif /* GSCALE_H_ */
