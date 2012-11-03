/*
 * exception.h
 *
 */

#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#define OnBeforeThrow(e) GScale_Exception_OnBeforeThrow((e))

#include "cexcept/cexcept.h"
#include "debug.h"

typedef struct _GScale_Exception{
    char *message;
    int code;
    const char *file;
    int line;
    const char *function;

    char trace[GSCALE_TRACE_SIZE][GSCALE_MAXIDENTIFIER];
} GScale_Exception;

#define EmptyException {"", 0, __FILE__, __LINE__, __func__, {""}}

define_exception_type(GScale_Exception);
extern struct exception_context the_exception_context[1];

void GScale_Exception_OnBeforeThrow(volatile GScale_Exception *except);

#define ThrowException(m, c) {GScale_Exception gsce = EmptyException; gsce.message = (char*)(m); gsce.code = (c); Throw gsce;}
#define ThrowErrnoOn(expr) {if((expr)){ ThrowException(strerror(errno), errno); } }
#define ThrowInvalidArgumentException() ThrowException("invalid argument given", EINVAL)

#endif /* EXCEPTION_H_ */
