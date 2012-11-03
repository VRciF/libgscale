/*
 * multireaderringbuffer.h
 *
 */

#ifndef MULTIREADERRINGBUFFER_H_
#define MULTIREADERRINGBUFFER_H_

#include <stdint.h>

#include "datastructure/doublelinkedlist.h"
#include "datastructure/ringbuffer.h"

typedef struct _multireaderRingBuffer{
    ringBuffer *rb;
    ringBufferMeta *origrbmeta;
    DoubleLinkedList readerlist;
} mrRingBufferMeta;

typedef struct _mrRingBufferReader {
    ringBufferMeta rbmeta;
    uint32_t beginreadpos;

    mrRingBufferMeta *mrrb;
} mrRingBufferReader;

void mrRingBuffer_init(mrRingBufferMeta *meta, ringBuffer *rb) /* throw (GScale_Exception) */;
mrRingBufferReader* mrRingBuffer_initReader(mrRingBufferMeta *meta) /* throw (GScale_Exception) */;
void mrRingBuffer_free(mrRingBufferMeta *meta) /* throw (GScale_Exception) */;
ringBuffer* mrRingBuffer_getRingBuffer(mrRingBufferReader *reader) /* throw (GScale_Exception) */;
void mrRingBuffer_switchToReader(mrRingBufferReader *reader) /* throw (GScale_Exception) */;
void mrRingBuffer_switchBackToRingBuffer(mrRingBufferReader *reader) /* throw (GScale_Exception) */;

#endif /* MULTIREADERRINGBUFFER_H_ */
