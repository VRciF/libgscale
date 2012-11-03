#ifndef MULTIREADERRINGBUFFER_C_
#define MULTIREADERRINGBUFFER_C_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "util/log.h"
#include "constants.h"
#include "exception.h"
#include "datastructure/ringbuffer.h"
#include "datastructure/multireaderringbuffer.h"

void mrRingBuffer_init(mrRingBufferMeta *meta, ringBuffer *rb) /* throw (GScale_Exception) */{
    if (meta == NULL || rb==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    meta->rb = rb;
    DoubleLinkedList_init(&meta->readerlist);
}

mrRingBufferReader* mrRingBuffer_initReader(mrRingBufferMeta *meta) /* throw (GScale_Exception) */{
    mrRingBufferReader *reader = NULL;
    GScale_Exception except;

    if (meta==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    Try{
        reader = DoubleLinkedList_allocNode(&meta->readerlist, sizeof(mrRingBufferReader));

        reader->mrrb = meta;

        ringBuffer_copyMetaSettings(ringBuffer_getMetaPointer(reader->mrrb->rb), &reader->rbmeta,
                                   GSCALE_RINGBUFFER_OPTION_WRITEPOS|GSCALE_RINGBUFFER_OPTION_READPOS|GSCALE_RINGBUFFER_OPTION_LENGTH);
    }Catch(except){
        if(reader!=NULL){
            DoubleLinkedList_freeNode(&meta->readerlist, reader);
        }

        Throw(except);
    }
    reader->rbmeta.readpos = reader->rbmeta.writepos;

    return reader;
}

void mrRingBuffer_free(mrRingBufferMeta *meta) /* throw (GScale_Exception) */{
    if (meta==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

	DoubleLinkedList_freeNodes(&meta->readerlist);
}

ringBuffer* mrRingBuffer_getRingBuffer(mrRingBufferReader *reader) /* throw (GScale_Exception) */{
    if (reader==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    return reader->mrrb->rb;
}

void mrRingBuffer_switchToReader(mrRingBufferReader *reader) /* throw (GScale_Exception) */{
    if (reader==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    reader->mrrb->origrbmeta = (ringBufferMeta*) ringBuffer_getMetaPointer(reader->mrrb->rb);

    /* the writepos tells us if the readpos has moved or not */

    reader->beginreadpos = ringBuffer_getReadPos(&reader->rbmeta);

    /* update writepos and length of current reader */
    ringBuffer_copyMetaSettings(reader->mrrb->origrbmeta, &reader->rbmeta, GSCALE_RINGBUFFER_OPTION_WRITEPOS|GSCALE_RINGBUFFER_OPTION_LENGTH);
    /* set reader metadata to those of ringbuffer */
    ringBuffer_initMetaPointer(reader->mrrb->rb, &reader->rbmeta);
}
void mrRingBuffer_switchBackToRingBuffer(mrRingBufferReader *reader) /* throw (GScale_Exception) */{
    uint32_t writepos = 0;
    uint32_t origreadpos = 0;
    uint32_t readpos = 0;

    if (reader==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    writepos = ringBuffer_getWritePos(&reader->rbmeta);
    origreadpos = ringBuffer_getReadPos(reader->mrrb->origrbmeta);
    readpos = ringBuffer_getReadPos(&reader->rbmeta);

    /* if readpos has moved */
    if(reader->beginreadpos != readpos && !((origreadpos<reader->beginreadpos && reader->beginreadpos<writepos) || (reader->beginreadpos<writepos && writepos<origreadpos))){
        /* now here comes the tricky part
         * we need to check if this read operation causes the original readbuffer to move readpos
         * and how far
         * so the last reader would move the orignial readbuffer
         */
        /* next we iterate through all reader's metadata to compare readpositions */
        mrRingBufferReader *tmpreader = NULL;

        tmpreader = (mrRingBufferReader*)DoubleLinkedList_iterBegin(&reader->mrrb->readerlist);
        while(DoubleLinkedList_iterIsValid(tmpreader)){
            /* so we're searching for the minimal valid readpos */
            uint32_t tmpreadpos = ringBuffer_getReadPos(&tmpreader->rbmeta);
            if(tmpreadpos<readpos && readpos<writepos){
                readpos = tmpreadpos;
            }
            else if(readpos<writepos && writepos<tmpreadpos){
                readpos = tmpreadpos;
            }

            tmpreader = (mrRingBufferReader*)DoubleLinkedList_iterNext(tmpreader);
        }

        if(readpos != origreadpos){
            reader->mrrb->origrbmeta->readpos = readpos;
        }
    }
    if(reader->rbmeta.writepos != reader->mrrb->origrbmeta->writepos){
        reader->mrrb->origrbmeta->writepos = reader->rbmeta.writepos;
    }

    /* set backup metadata as ringbuffer metadata */
    ringBuffer_initMetaPointer(reader->mrrb->rb, reader->mrrb->origrbmeta);
}

#endif /* MULTIREADERRINGBUFFER_H_ */
