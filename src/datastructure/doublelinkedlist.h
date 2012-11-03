/*
 * doublelinkedlist.h
 *
 *  Created on: Oct 20, 2010
 */

#ifndef DOUBLELINKEDLIST_H_
#define DOUBLELINKEDLIST_H_

#include <stdint.h>

/* WEIRD IDEA FOR THE DOUBLE LINKED LIST:
 * given some arbitrary data, one need to allocate and store
 * you have to use the functions provided by the doublelinkedlist to allocate memory
 * for your arbitrary data
 * the reason is that your allocated data is placed right after a nodes header
 * this means, if you have some data floating around in the application
 * you immediately know where this data is within the list
 *
 * one idea behind this implementation is that you either have one exact value which you
 * want operate on e.g. free()
 * or you have the list and iterate over all nodes
 * so a search shall not be needed
 */

typedef struct _doubleLinkedListNode DoubleLinkedListNode;

struct _doubleLinkedListNode {
    DoubleLinkedListNode *prev;
    DoubleLinkedListNode *next;

    void *data;
};

typedef struct {
    uint32_t nodecount;
    DoubleLinkedListNode *tail;
    DoubleLinkedListNode *root;
} DoubleLinkedList;

DoubleLinkedListNode* DoubleLinkedList_getNodeByData(const void *data) /* throw (GScale_Exception) */;
DoubleLinkedList* DoubleLinkedList_allocAndInit() /* throw (GScale_Exception) */;
void DoubleLinkedList_free(DoubleLinkedList *list) /* throw (GScale_Exception) */;
void DoubleLinkedList_freeNodes(DoubleLinkedList *list) /* throw (GScale_Exception) */;
void DoubleLinkedList_init(DoubleLinkedList *list) /* throw (GScale_Exception) */;
void* DoubleLinkedList_allocNode(DoubleLinkedList *list, uint32_t datasize) /* throw (GScale_Exception) */;
void DoubleLinkedList_freeNode(DoubleLinkedList *list, void *data) /* throw (GScale_Exception) */;
int8_t DoubleLinkedList_isEmpty(const DoubleLinkedList *list) /* throw (GScale_Exception) */;
void* DoubleLinkedList_iterBegin(const DoubleLinkedList *list) /* throw (GScale_Exception) */;
int8_t DoubleLinkedList_iterIsValid(const void *data);
void* DoubleLinkedList_iterNext(const void *data) /* throw (GScale_Exception) */;
uint32_t DoubleLinkedList_countNodes(const DoubleLinkedList *list);

#endif /* LIST_H_ */
