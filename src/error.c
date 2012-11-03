#ifndef ERROR_C_
#define ERROR_C_

#include "error.h"
#include "exception.h"

GScale_Error* GScale_getLastError(){
    static GScale_Error error = { {0, NULL, 0}, EmptyException, 0, "", "", "" };
    return &error;
}

/*************************** Settings ***********************************/

GScale_ErrorSettings* GScale_getErrorSettings(){
    static GScale_ErrorSettings settings = { NULL, 0 };
    return &settings;
}

GScale_ErrorSettings* GScale_setErrorHandler(GScale_ErrorHandler errorHandler){
	GScale_ErrorSettings* settings = GScale_getErrorSettings();
	settings->errorHandler = errorHandler;
	return settings;
}
GScale_ErrorSettings* GScale_enableTrace(){
	GScale_ErrorSettings* settings = GScale_getErrorSettings();
	settings->trace = 1;
	return settings;
}
GScale_ErrorSettings* GScale_disableTrace(){
	GScale_ErrorSettings* settings = GScale_getErrorSettings();
	settings->trace = 0;
	return settings;
}

#endif
