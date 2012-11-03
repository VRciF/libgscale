#ifndef ERROR_H_
#define ERROR_H_

#include <stdint.h>
#include <stddef.h>

#include "exception.h"

typedef struct _GScale_Error GScale_Error;
struct _GScale_Error {
    struct {
        int code;

        char *file;
        int line;
    } system;
    GScale_Exception except;

    uint8_t is_permanent_error; /* e.g. if something is wrongly configured */
    char cause[2048];
    char consequence[1024];
    char solution[2048];
};

#define GSCALE_ERROR_SYSTEM 1
#define GSCALE_ERROR_INTERN 2

typedef void (*GScale_ErrorHandler)(GScale_Error *);

typedef struct _GScale_ErrorSettings GScale_ErrorSettings;
struct _GScale_ErrorSettings {
    GScale_ErrorHandler errorHandler;
    uint8_t trace;
};

GScale_Error* GScale_getLastError();
GScale_Error* GScale_setErrorCode(int type, int code);
GScale_Error* GScale_setErrorLocation(int type, char *file, int line);
GScale_Error* GScale_setErrorMessage(char *msg, ...);
GScale_Error* GScale_setErrorPermanent(uint8_t yesno);
GScale_Error* GScale_setErrorCause(char *cause, ...);
GScale_Error* GScale_setErrorConsequence(char *cons, ...);
GScale_Error* GScale_setErrorSolution(char *sol, ...);

GScale_ErrorSettings* GScale_getErrorSettings();

GScale_ErrorSettings* GScale_setErrorHandler(GScale_ErrorHandler errorHandler);
GScale_ErrorSettings* GScale_enableTrace();
GScale_ErrorSettings* GScale_disableTrace();

#endif
