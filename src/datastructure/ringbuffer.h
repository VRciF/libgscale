/*
 * ringbuffer.h
 *
 *  Created on: Oct 16, 2010
 */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <stdint.h>

#define GSCALE_RINGBUFFER_OPTION_VOID         1  /* void option */
#define GSCALE_RINGBUFFER_OPTION_SKIPDATA     2  /* only move offsets of the ringbuffer */
#define GSCALE_RINGBUFFER_OPTION_SKIPOFFSET   4  /* only change the buffers data without modifying offsets */
#define GSCALE_RINGBUFFER_OPTION_ALLORNOTHING 8  /* do the operation for all available data or nothing */

#define GSCALE_RINGBUFFER_OPTION_WRITEPOS 16  /* use writepos for operation */
#define GSCALE_RINGBUFFER_OPTION_READPOS  32  /* use readpos for operation */
#define GSCALE_RINGBUFFER_OPTION_LENGTH   64  /* use length for operation */

#define GSCALE_RINGBUFFER_INIT_KEEPSETTINGS 2
#define GSCALE_RINGBUFFER_INIT_RESET 3

typedef struct _ringBufferMeta{
    uint32_t readpos;
    uint32_t writepos;
    uint32_t length;
} ringBufferMeta;

typedef struct _ringBuffer {
    /*
     * im not using
     * char data[]
     * because maybe in future ill try to implement a more intelligent
     * ringbuffer
     * with intelligent i mean, that the ringbuffer can resize itself if needed
     * e.g. if the buffer is full pretty often (not yet sure how to measure that)
     * it could resize itself
     * but this one need to be very deterministic in sense that no wobly traffic effects shall occure
     */
    char *data;

    /* buffers metadata is accessed by *meta because
     * if the buffer is used by 2 processes via shared memory the *data can be on different locations
     * but within this location the metadata needs to be shared (in the shared memory)
     * so its easier to just let both parts of the buffer (data, meta) just point to the loction
     * where they are
     * it provides much more flexibility with the cost of usability
     */
    ringBufferMeta *meta;
} ringBuffer;

typedef struct _rbmMap {
    void *mappedregion;
    uint32_t mappedlength;

    uint32_t writepos;
} ringBufferMemMap;

void ringBuffer_initMetaPointer(ringBuffer *buffer, ringBufferMeta *meta) /* throw (GScale_Exception) */;
void ringBuffer_initDataPointer(ringBuffer *buffer, uint32_t datalength, char *data) /* throw (GScale_Exception) */;
void ringBuffer_copyMetaSettings(const ringBufferMeta *src, ringBufferMeta *dst, int16_t options) /* throw (GScale_Exception) */;
void ringBuffer_reset(ringBuffer *buffer) /* throw (GScale_Exception) */;
const ringBufferMeta* ringBuffer_getMetaPointer(ringBuffer *buffer) /* throw (GScale_Exception) */;
uint32_t ringBuffer_getReadPos(const ringBufferMeta *meta) /* throw (GScale_Exception) */;
uint32_t ringBuffer_getWritePos(const ringBufferMeta *meta) /* throw (GScale_Exception) */;
uint32_t ringBuffer_getLength(const ringBufferMeta *meta) /* throw (GScale_Exception) */;

int8_t ringBuffer_isFull(const ringBuffer *buffer) /* throw (GScale_Exception) */;
int8_t ringBuffer_isEmpty(const ringBuffer *buffer) /* throw (GScale_Exception) */;

int8_t ringBuffer_canWrite(const ringBuffer *buffer) /* throw (GScale_Exception) */;
int8_t ringBuffer_canRead(const ringBuffer *buffer) /* throw (GScale_Exception) */;

uint32_t ringBuffer_getFIONREAD(const ringBuffer *buffer) /* throw (GScale_Exception) */;
uint32_t ringBuffer_getFIONWRITE(const ringBuffer *buffer) /* throw (GScale_Exception) */;
int32_t ringBuffer_rollbackWrite(ringBuffer *buffer, uint32_t length) /* throw (GScale_Exception) */;
int32_t ringBuffer_write(ringBuffer *buffer, void *data, int32_t length);
int32_t ringBuffer_writeExt(ringBuffer *buffer, void *data, int32_t length,int16_t options);
int32_t ringBuffer_multiWrite(ringBuffer *buffer, void **datap, int32_t *lengthp);
int32_t ringBuffer_multiWriteExt(ringBuffer *buffer, void **datap, int32_t *lengthp, int16_t options) /* throw (GScale_Exception) */;

int32_t ringBuffer_read(ringBuffer *buffer, char *data, uint32_t length);
int32_t ringBuffer_readExt(ringBuffer *buffer, char *data, uint32_t length, int16_t options) /* throw (GScale_Exception) */;

int32_t ringBuffer_dmaWrite(ringBuffer *buffer, void **data) /* throw (GScale_Exception) */;
int32_t ringBuffer_dmaRead(ringBuffer *buffer, void **data) /* throw (GScale_Exception) */;

int8_t ringBuffer_initMemMap(ringBuffer *buffer, ringBufferMemMap *map,
        void *mappedregion, uint32_t mappedlength) /* throw (GScale_Exception) */;
int8_t ringBuffer_writeMemMap(ringBuffer *buffer, ringBufferMemMap *map,
        int16_t options) /* throw (GScale_Exception) */;

#endif /* RINGBUFFER_H_ */
