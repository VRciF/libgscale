/*
 * doublelinkedlist.h
 *
 *  Created on: Oct 20, 2010
 */

#ifndef DOUBLELINKEDLIST_C_
#define DOUBLELINKEDLIST_C_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "util/log.h"
#include "constants.h"
#include "exception.h"
#include "datastructure/doublelinkedlist.h"

/* BEGIN: protected - you shouldnt call this functions from your code */

DoubleLinkedListNode* DoubleLinkedList_getNodeByData(const void *data) /* throw (GScale_Exception) */{
    DoubleLinkedListNode *node = NULL;

    if (data == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    node = (DoubleLinkedListNode*) (((char*) data)
            - sizeof(DoubleLinkedListNode));
    return node;
}

/* END: protected */

/* BEGIN: public */

/*#if defined(HAVE_MALLOC_H) || defined(HAVE_STDLIB_H)*/

DoubleLinkedList* DoubleLinkedList_allocAndInit() /* throw (GScale_Exception) */{

    DoubleLinkedList *list = (DoubleLinkedList*)malloc(sizeof(DoubleLinkedList));
    if(list == NULL) {
        ThrowException("allocation failed", errno);
    }

    DoubleLinkedList_init(list);
    return list;
}

void DoubleLinkedList_free(DoubleLinkedList *list) /* throw (GScale_Exception) */{
    if(list == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    DoubleLinkedList_freeNodes(list);
    free(list);
}

void DoubleLinkedList_freeNodes(DoubleLinkedList *list) /* throw (GScale_Exception) */{
    if(list == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    while(list->root) {
        DoubleLinkedList_freeNode(list, list->root->data);
    }
}

/*#endif*/

void DoubleLinkedList_init(DoubleLinkedList *list) /* throw (GScale_Exception) */{
    if (list == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    list->nodecount = 0;
    list->tail = NULL;
    list->root = NULL;
}

void* DoubleLinkedList_allocNode(DoubleLinkedList *list, uint32_t datasize) /* throw (GScale_Exception) */{
    DoubleLinkedListNode *node = NULL;
    if (list == NULL || datasize <= 0) {
        ThrowException("invalid argument given", EINVAL);
    }

    node = (DoubleLinkedListNode*) calloc(sizeof(DoubleLinkedListNode), datasize);
    if(node==NULL){
        ThrowException("memory allocation failed", errno);
    }

    node->next = NULL;
    node->prev = list->tail;
    if (node->prev != NULL) {
        node->prev->next = node;
    }
    list->tail = node;
    if (list->root == NULL) {
        list->root = node;
    }

    node->data = (void*) (((char*) node) + sizeof(DoubleLinkedListNode));
    list->nodecount++;

    return node->data;
}

void DoubleLinkedList_freeNode(DoubleLinkedList *list, void *data) /* throw (GScale_Exception) */{
    DoubleLinkedListNode *node = NULL;
    if (list == NULL || data == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    node = DoubleLinkedList_getNodeByData(data);
    if (node->prev == NULL) {
        list->root = node->next;
    } else {
        node->prev->next = node->next;
    }
    if (node->next == NULL) {
        list->tail = node->prev;
    } else {
        node->next->prev = node->prev;
    }

    free(node);
    list->nodecount--;
}

int8_t DoubleLinkedList_isEmpty(const DoubleLinkedList *list) /* throw (GScale_Exception) */{
    if (list == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    return list->root == NULL ? GSCALE_OK : GSCALE_NOK;
}

void* DoubleLinkedList_iterBegin(const DoubleLinkedList *list) /* throw (GScale_Exception) */{
    if (list == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    if (list->root == NULL) {
        return NULL;
    }
    return list->root->data;
}

int8_t DoubleLinkedList_iterIsValid(const void *data) {
    if (data == NULL) {
        return GSCALE_NOK;
    }

    return GSCALE_OK;
}

void* DoubleLinkedList_iterNext(const void *data) /* throw (GScale_Exception) */{
    DoubleLinkedListNode *node = NULL;
    if (data == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    node = DoubleLinkedList_getNodeByData(data);
    if(node->next==NULL){
        return NULL;
    }

    return node->next->data;
}

uint32_t DoubleLinkedList_countNodes(const DoubleLinkedList *list){
    if(list==NULL){ return 0; }
	return list->nodecount;
}

/* END: public */

#endif /* LIST_H_ */
