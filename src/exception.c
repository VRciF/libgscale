/*
 * exception.c
 */

#ifndef EXCEPTION_C_
#define EXCEPTION_C_

#include "exception.h"
#include "error.h"
#include "debug.h"

struct exception_context the_exception_context[1];

void GScale_Exception_OnBeforeThrow(volatile GScale_Exception *except){
	GScale_Error* error = GScale_getLastError();

	GScale_Debug_FillTrace((char(*)[256])except->trace, sizeof(except->trace[0]));

	error->except = *except;
	GScale_getErrorSettings()->errorHandler(error);
}

#endif
