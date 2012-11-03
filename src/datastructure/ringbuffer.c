#ifndef RINGBUFFER_C_
#define RINGBUFFER_C_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "util/log.h"
#include "constants.h"
#include "exception.h"
#include "datastructure/ringbuffer.h"

void ringBuffer_initMetaPointer(ringBuffer *buffer, ringBufferMeta *meta) /* throw (GScale_Exception) */{
    if(buffer == NULL || meta == NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    buffer->meta = meta;
}

void ringBuffer_initDataPointer(ringBuffer *buffer, uint32_t datalength, char *data) /* throw (GScale_Exception) */{
    if(buffer == NULL || datalength==0){
        ThrowException("invalid argument given", EINVAL);
    }
    if(buffer->meta == NULL){
        ThrowException("metapointer is needed before datapointer", EINVAL); /* because meta->length = ... */
    }

    if(data==NULL){
        buffer->data = ((char*)buffer)+sizeof(ringBuffer);
        buffer->meta->length = datalength;
    }
    else{
        buffer->data = data;
        buffer->meta->length = datalength;
    }
}

void ringBuffer_copyMetaSettings(const ringBufferMeta *src, ringBufferMeta *dst, int16_t options) /* throw (GScale_Exception) */{
    if(src == NULL || dst == NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    if((options & GSCALE_RINGBUFFER_OPTION_WRITEPOS) == GSCALE_RINGBUFFER_OPTION_WRITEPOS){
        dst->writepos = src->writepos;
    }
    if((options & GSCALE_RINGBUFFER_OPTION_READPOS) == GSCALE_RINGBUFFER_OPTION_READPOS){
        dst->readpos = src->readpos;
    }
    if((options & GSCALE_RINGBUFFER_OPTION_LENGTH) == GSCALE_RINGBUFFER_OPTION_LENGTH){
        dst->length = src->length;
    }
}

void ringBuffer_reset(ringBuffer *buffer) /* throw (GScale_Exception) */{
    if(buffer == NULL || buffer->meta==NULL){
        ThrowException("invalid argument given", EINVAL);
    }

    buffer->meta->readpos = 0;
    buffer->meta->writepos = 0;
}

const ringBufferMeta* ringBuffer_getMetaPointer(ringBuffer *buffer) /* throw (GScale_Exception) */{
    if (buffer == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    return buffer->meta;
}

uint32_t ringBuffer_getReadPos(const ringBufferMeta *meta) /* throw (GScale_Exception) */{
    if (meta==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    return meta->readpos;
}
uint32_t ringBuffer_getWritePos(const ringBufferMeta *meta) /* throw (GScale_Exception) */{
    if (meta==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    return meta->writepos;
}
uint32_t ringBuffer_getLength(const ringBufferMeta *meta) /* throw (GScale_Exception) */{
    if (meta == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    return meta->length;
}

/* begin internal functions */
int8_t ringBuffer_isFull(const ringBuffer *buffer) /* throw (GScale_Exception) */{
    if (buffer == NULL || buffer->meta==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    /* the buffer is full if writepos is right before readpos */
    if (buffer->meta->writepos + 1 == buffer->meta->readpos ||
        (buffer->meta->readpos == 0 && buffer->meta->writepos == buffer->meta->length - 1)) {
        return 1;
    }
    return 0;
}
int8_t ringBuffer_isEmpty(const ringBuffer *buffer) /* throw (GScale_Exception) */{
    if (buffer == NULL || buffer->meta==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    return buffer->meta->readpos == buffer->meta->writepos;
}
/* end internal functions */

/* begin public functions*/
int8_t ringBuffer_canWrite(const ringBuffer *buffer) /* throw (GScale_Exception) */{
    if (buffer == NULL || buffer->meta==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    return !ringBuffer_isFull(buffer);
}
int8_t ringBuffer_canRead(const ringBuffer *buffer) /* throw (GScale_Exception) */{
    if (buffer == NULL || buffer->meta==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    return !ringBuffer_isEmpty(buffer);
}

uint32_t ringBuffer_getFIONREAD(const ringBuffer *buffer) /* throw (GScale_Exception) */{
    if (buffer == NULL || buffer->meta==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    if (ringBuffer_isEmpty(buffer)) {
        return 0;
    }

    if (buffer->meta->readpos < buffer->meta->writepos) {
        return buffer->meta->writepos - buffer->meta->readpos;
    } else {
        return buffer->meta->length - buffer->meta->readpos + buffer->meta->writepos;
    }
}
uint32_t ringBuffer_getFIONWRITE(const ringBuffer *buffer) /* throw (GScale_Exception) */{
    if (buffer == NULL || buffer->meta==NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    if (ringBuffer_isFull(buffer)) {
        return 0;
    }

    if (buffer->meta->writepos < buffer->meta->readpos) {
        return buffer->meta->readpos - buffer->meta->writepos - 1;
    } else {
        return buffer->meta->length - buffer->meta->writepos + buffer->meta->readpos - 1;
    }
}

/**
 * this one gets complicated
 * its a reverse write, which means i need to move the writeoffset backwards
 * accordingly
 */
int32_t ringBuffer_rollbackWrite(ringBuffer *buffer, uint32_t length) /* throw (GScale_Exception) */{
    if (buffer == NULL || buffer->meta==NULL || length <= 0) {
        ThrowException("invalid argument given", EINVAL);
    }
    if (ringBuffer_isEmpty(buffer)) {
        ThrowException("buffer is empty", ENOBUFS);
    }
    /* here we can be sure that there is space to reverse write some data */

    if (buffer->meta->readpos < buffer->meta->writepos) {
        /* writing backwards can be done at max buffer->meta->readpos+1 */
        if (buffer->meta->writepos - (buffer->meta->readpos + 1) < length) {
            length = buffer->meta->writepos - (buffer->meta->readpos + 1);
        }
        buffer->meta->writepos -= length;
    } else {
        /* this means readpos > writepos -and readpos!=writepos (cause buffer is not empty) */

        if (length < buffer->meta->writepos) {
            buffer->meta->writepos -= length;
        } else {
            /*  here, the rollback will wrap around the buffer limits */
            uint32_t written = 0;
            /* readpos is at the limit of the buffer */
            length -= buffer->meta->writepos;
            written += buffer->meta->writepos;
            buffer->meta->writepos = 0;
            if (buffer->meta->readpos != buffer->meta->length) {

                /* wrap around */
                if (buffer->meta->length - (buffer->meta->readpos + 1) < length) {
                    length = buffer->meta->length - (buffer->meta->readpos + 1);
                }

                if (length > 0) {
                    buffer->meta->writepos = buffer->meta->length - length;
                    written += buffer->meta->length - length;
                }
                length = written;

            }
        }

    }
    return length;
}

int32_t ringBuffer_write(ringBuffer *buffer, void *data, int32_t length){
    return ringBuffer_writeExt(buffer, data, length,GSCALE_RINGBUFFER_OPTION_VOID);
}

int32_t ringBuffer_writeExt(ringBuffer *buffer, void *data, int32_t length, int16_t options) {
    void *tmpd[2] = { NULL, NULL };
    int32_t tmpl[2] = { 0, 0 };

    tmpd[0] = data;
    tmpl[0] = length;
    return ringBuffer_multiWriteExt(buffer, tmpd, tmpl, options);
}

int32_t ringBuffer_multiWrite(ringBuffer *buffer, void **datap, int32_t *lengthp){
    return ringBuffer_multiWriteExt(buffer, datap, lengthp, GSCALE_RINGBUFFER_OPTION_ALLORNOTHING);
}

int32_t ringBuffer_multiWriteExt(ringBuffer *buffer, void **datap, int32_t *lengthp, int16_t options) /* throw (GScale_Exception) */{

    uint16_t i = 0;
    uint32_t bwritepos = 0;
    int32_t lengthwritten = 0;
    uint8_t allornothing = (options & GSCALE_RINGBUFFER_OPTION_ALLORNOTHING)
            == GSCALE_RINGBUFFER_OPTION_ALLORNOTHING;

    if (buffer == NULL || buffer->meta==NULL || ((datap == NULL || *datap == NULL) && (options
            & GSCALE_RINGBUFFER_OPTION_SKIPDATA)
            != GSCALE_RINGBUFFER_OPTION_SKIPDATA)) {
        ThrowException("invalid argument given", EINVAL);
    }
    if (ringBuffer_isFull(buffer)) {
        ThrowException("buffer is full", ENOBUFS);
    }
    bwritepos = buffer->meta->writepos;

    for (i = 0; datap[i] != NULL; i++) {
        char *data = datap[i];
        uint32_t length = lengthp[i];

        if (allornothing && ringBuffer_getFIONWRITE(buffer) < length) {
            ThrowException("not enough buffer", ENOBUFS);
        }

        /* here we can be sure that there is space to write some data */

        /* writes can be done at max until writepos is right before readpos := buffer is full */
        if (buffer->meta->readpos < bwritepos) {
            /* data could wrap around */
            /* -1 to be sure we stop 1 position before readpos := buffer full*/
            uint32_t lengthcanbewritten = (buffer->meta->length - bwritepos)
                    + (buffer->meta->readpos - 1);
            /* at this line lengthtoread equals the max length which can be read */
            length = lengthcanbewritten > (uint32_t) length ? length
                    : lengthcanbewritten;

            if (buffer->meta->length - bwritepos >= length) {
                if ((options & GSCALE_RINGBUFFER_OPTION_SKIPDATA)
                        != GSCALE_RINGBUFFER_OPTION_SKIPDATA) {
                    if (memcpy(buffer->data + bwritepos, data, length) == NULL) {
                        if (allornothing) {
                            ThrowException("copy failed", ENOBUFS);
                        } else {
                            if ((options & GSCALE_RINGBUFFER_OPTION_SKIPOFFSET)
                                    != GSCALE_RINGBUFFER_OPTION_SKIPOFFSET){
                                buffer->meta->writepos = bwritepos;
                            }
                            return lengthwritten;
                        }
                    }
                }
                bwritepos += length;
                if (bwritepos >= buffer->meta->length && buffer->meta->readpos != 0) {
                    bwritepos = 0;
                }
                lengthwritten = length;
                length = 0;
            } else {
                if ((options & GSCALE_RINGBUFFER_OPTION_SKIPDATA)
                        != GSCALE_RINGBUFFER_OPTION_SKIPDATA) {
                    if (memcpy(buffer->data + bwritepos, data, buffer->meta->length
                            - bwritepos) == NULL) {
                        if (allornothing) {
                            ThrowException("copy failed", ENOBUFS);
                        } else {
                            if ((options & GSCALE_RINGBUFFER_OPTION_SKIPOFFSET)
                                    != GSCALE_RINGBUFFER_OPTION_SKIPOFFSET){
                                buffer->meta->writepos = bwritepos;
                            }
                            return lengthwritten;
                        }
                    }
                }
                lengthwritten = buffer->meta->length - bwritepos;

                data += lengthwritten;
                length -= lengthwritten;

                bwritepos = 0;

            }
        }

        if (length > 0) {
            /* just write to buffer */
            uint32_t lengthcanbewritten = 0;
            if (ringBuffer_isEmpty(buffer)) {
                lengthcanbewritten = buffer->meta->length;
            } else {
                lengthcanbewritten = (buffer->meta->readpos - 1) - bwritepos;
            }

            /* at this line lengthtoread equals the max length which can be read */
            length = lengthcanbewritten > length ? length : lengthcanbewritten;

            /* memcpy buffer and move readpos */
            if ((options & GSCALE_RINGBUFFER_OPTION_SKIPDATA)
                    != GSCALE_RINGBUFFER_OPTION_SKIPDATA) {
                if (memcpy(buffer->data + bwritepos, data, length) == NULL) {
                    if (allornothing) {
                        ThrowException("copy failed", ENOBUFS);
                    } else {
                        if ((options & GSCALE_RINGBUFFER_OPTION_SKIPOFFSET)
                                != GSCALE_RINGBUFFER_OPTION_SKIPOFFSET){
                            buffer->meta->writepos = bwritepos;
                        }
                        return lengthwritten;
                    }
                }
            }

            bwritepos += length;
            if (lengthwritten < 0) {
                lengthwritten = 0;
            }
            lengthwritten += length;
        }
    }
    if ((options & GSCALE_RINGBUFFER_OPTION_SKIPOFFSET)
            != GSCALE_RINGBUFFER_OPTION_SKIPOFFSET){
        buffer->meta->writepos = bwritepos;
    }

    return lengthwritten;
}

int32_t ringBuffer_read(ringBuffer *buffer, char *data, uint32_t length){
    return ringBuffer_readExt(buffer, data, length, GSCALE_RINGBUFFER_OPTION_VOID);
}

int32_t ringBuffer_readExt(ringBuffer *buffer, char *data, uint32_t length, int16_t options) /* throw (GScale_Exception) */{
    int32_t lengthread = 0;
    uint32_t breadpos = 0;
    uint32_t lengthcanberead = 0;
    uint8_t allornothing = (options & GSCALE_RINGBUFFER_OPTION_ALLORNOTHING)
            == GSCALE_RINGBUFFER_OPTION_ALLORNOTHING;

    if (buffer == NULL || buffer->meta==NULL || (data == NULL && (options
            & GSCALE_RINGBUFFER_OPTION_SKIPDATA)
            != GSCALE_RINGBUFFER_OPTION_SKIPDATA) || length <= 0) {
        ThrowException("invalid argument given", EINVAL);
    }
    if (ringBuffer_isEmpty(buffer)) {
        ThrowException("buffer is empty", ENOBUFS);
    }
    if (allornothing && ringBuffer_getFIONREAD(buffer) < length) {
        ThrowException("not enough space", ENOBUFS);
    }
    /* here we can be sure that readpos != writepos */

    /* read's can be done until readpos = writepos := buffer is empty */
    breadpos = buffer->meta->readpos;

    if (breadpos > buffer->meta->writepos) {
        lengthcanberead = (buffer->meta->length - breadpos)
                + buffer->meta->writepos;
        /* at this line lengthtoread equals the max length which can be read */
        length = lengthcanberead > (uint32_t) length ? length : lengthcanberead;

        if (buffer->meta->length - breadpos >= length) {
            if ((options & GSCALE_RINGBUFFER_OPTION_SKIPDATA)
                    != GSCALE_RINGBUFFER_OPTION_SKIPDATA) {
                if (memcpy(data, buffer->data + breadpos, length) == NULL) {
                    if (allornothing) {
                        ThrowException("copy failed", ENOBUFS);
                    } else {
                        if ((options & GSCALE_RINGBUFFER_OPTION_SKIPOFFSET)
                                != GSCALE_RINGBUFFER_OPTION_SKIPOFFSET){
                            buffer->meta->readpos = breadpos;
                        }
                        return lengthread;
                    }
                }
            }
            breadpos += length;
            if (breadpos >= buffer->meta->length) {
                breadpos = 0;
            }
            lengthread = length;
            length = 0;
        } else {
            if ((options & GSCALE_RINGBUFFER_OPTION_SKIPDATA)
                    != GSCALE_RINGBUFFER_OPTION_SKIPDATA) {
                if (memcpy(data, buffer->data + breadpos, buffer->meta->length
                        - breadpos) == NULL) {
                    if (allornothing) {
                        ThrowException("copy failed", ENOBUFS);
                    } else {
                        if ((options & GSCALE_RINGBUFFER_OPTION_SKIPOFFSET)
                                != GSCALE_RINGBUFFER_OPTION_SKIPOFFSET){
                            buffer->meta->readpos = breadpos;
                        }
                        return lengthread;
                    }
                }
            }
            lengthread = buffer->meta->length - breadpos;

            data += lengthread;
            length -= lengthread;

            breadpos = 0;

        }
    }

    if (length > 0) {
        lengthcanberead = buffer->meta->writepos - breadpos;
        /* at this line lengthtoread equals the max length which can be read */
        length = lengthcanberead > length ? length : lengthcanberead;

        /* memcpy buffer and move readpos */
        if ((options & GSCALE_RINGBUFFER_OPTION_SKIPDATA)
                != GSCALE_RINGBUFFER_OPTION_SKIPDATA) {
            if (memcpy(data, buffer->data + breadpos, length) == NULL) {
                if (allornothing) {
                    ThrowException("copy failed", ENOBUFS);
                } else {
                    if ((options & GSCALE_RINGBUFFER_OPTION_SKIPOFFSET)
                            != GSCALE_RINGBUFFER_OPTION_SKIPOFFSET){
                        buffer->meta->readpos = breadpos;
                    }
                    return lengthread;
                }
            }
        }
        breadpos += length;
        if (lengthread < 0) {
            lengthread = 0;
        }
        lengthread += length;
    }
    if ((options & GSCALE_RINGBUFFER_OPTION_SKIPOFFSET)
            != GSCALE_RINGBUFFER_OPTION_SKIPOFFSET){
        buffer->meta->readpos = breadpos;
    }

    return lengthread;
}

/**
 * data is set to the buffer position where one can directly write into the buffer
 * the max length which shall be written is given by the return value
 *
 * this function is only destined for cases where performance is really really an issue
 * if ure unsure or lack knowledge of the language C or such
 * dont use it!
 * if ure an expert in high performance computing and C read the function to be sure
 * to use it correctly - i think its worth it!
 */
int32_t ringBuffer_dmaWrite(ringBuffer *buffer, void **data) /* throw (GScale_Exception) */{
    int32_t lengthwritten = 0;

    if (buffer == NULL || buffer->meta==NULL || data == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    if (ringBuffer_isFull(buffer)) {
    	return lengthwritten;
    }
    /* here we can be sure that there is space to write some data */

    /* writes can be done at max until writepos is right before readpos := buffer is full */

    *data = buffer->data + buffer->meta->writepos;
    if (buffer->meta->readpos < buffer->meta->writepos) {
        /* data could wrap around */
        /* -1 to be sure we stop 1 position before readpos := buffer full*/
        lengthwritten = (int32_t)((buffer->meta->length - buffer->meta->writepos)
                + (buffer->meta->readpos - 1));

    } else {
        /* just write to buffer */
        if (ringBuffer_isEmpty(buffer)) {
            lengthwritten = (int32_t) buffer->meta->length;
        } else {
            lengthwritten = (int32_t)((buffer->meta->readpos - 1) - buffer->meta->writepos);
        }
    }

    return lengthwritten;
}
/**
 * data is set to the buffer position where one can directly read from the buffer
 * the max length which shall be read is given by the return value
 *
 * this function is only destined for cases where performance is really really an issue
 * if ure unsure or lack knowledge of the language C or such
 * dont use it!
 * if ure an expert in high performance computing and C read the function to be sure
 * to use it correctly - i think its worth it!
 */
int32_t ringBuffer_dmaRead(ringBuffer *buffer, void **data) /* throw (GScale_Exception) */{
    int32_t lengthread = 0;

    if (buffer == NULL || buffer->meta==NULL || data == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }
    if (ringBuffer_isEmpty(buffer)) {
        return lengthread;
    }
    /* here we can be sure that readpos != writepos */

    /* read's can be done until readpos = writepos := buffer is empty */

    *data = buffer->data + buffer->meta->readpos;
    if (buffer->meta->readpos > buffer->meta->writepos) {
        lengthread = (int32_t)((buffer->meta->length - buffer->meta->readpos)
                + buffer->meta->writepos);
    } else {
        lengthread = (int32_t)(buffer->meta->writepos - buffer->meta->readpos);
    }

    return lengthread;
}

int8_t ringBuffer_initMemMap(ringBuffer *buffer, ringBufferMemMap *map,
    void *mappedregion, uint32_t mappedlength) /* throw (GScale_Exception) */{
    uint32_t fionwrite = 0;

    if (buffer == NULL || buffer->meta==NULL || map == NULL || mappedregion == NULL || mappedlength <= 0) {
        ThrowException("invalid argument given", EINVAL);
    }

    fionwrite = ringBuffer_getFIONWRITE(buffer);

    if (fionwrite < mappedlength) {
        ThrowException("not enough buffer", ENOSPC);
    }

    map->mappedregion = mappedregion;
    map->mappedlength = mappedlength;
    map->writepos = buffer->meta->readpos;

    return GSCALE_OK;
}
int8_t ringBuffer_writeMemMap(ringBuffer *buffer, ringBufferMemMap *map,
        int16_t options) /* throw (GScale_Exception) */{
    int32_t fionwrite_before = 0;
    uint32_t backupwritepos = 0;
    int32_t fionwrite_after = 0;

    if (buffer == NULL || buffer->meta==NULL || map == NULL) {
        ThrowException("invalid argument given", EINVAL);
    }

    fionwrite_before = ringBuffer_getFIONWRITE(buffer);

    backupwritepos = buffer->meta->writepos;

    buffer->meta->writepos = map->writepos;
    ringBuffer_writeExt(buffer, map->mappedregion, map->mappedlength, options);
    fionwrite_after = ringBuffer_getFIONWRITE(buffer);
    /* if the write caused less data written than there was already before in the buffer
     * we'll set writepos back to where it was before
     */
    if (fionwrite_after > fionwrite_before) {
        buffer->meta->writepos = backupwritepos;
    }

    return GSCALE_OK;
}

/* end public functions */

#endif /* RINGBUFFER_H_ */
