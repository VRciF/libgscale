#ifndef DEBUG_C_
#define DEBUG_C_


#include <execinfo.h>
#include <string.h>
#include <stdlib.h>

#include "debug.h"

/*
int *GSCALESTACKSTART = NULL;
int *GSCALECURRENTSTACK = NULL;

int GSCALE_TRACE_ENABLED = 1;

struct GScale_Trace GSCALE_BACKTRACE[GSCALE_MAXTRACE] = { {NULL,0,NULL, 0} };

int GSCALE_TRACE_CURRENT = 0;
*/

void GScale_Debug_FillTrace(char (*strings)[GSCALE_MAXIDENTIFIER], size_t ssize){
    void **array = NULL;
    size_t size;
    char **tmpstr;
    size_t i;
    size_t slen;

    for (i = 0; i < GSCALE_TRACE_SIZE; i++){
        memset((void*)(strings[i]), '\0', ssize);
    }

    array = calloc(GSCALE_TRACE_SIZE, sizeof(void*));
    if(array==NULL){ return; }
    size = backtrace (array, GSCALE_TRACE_SIZE);
    tmpstr = backtrace_symbols (array, size);
    for (i = 0; i < size; i++){
        slen = strlen(tmpstr[i]);
        if(slen >= ssize){
            slen = ssize-1;
        }
        memcpy((void*)(strings[i]), tmpstr[i], slen);
    }
    free(tmpstr);
    free(array);
}

#endif
