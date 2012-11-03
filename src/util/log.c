/*
 * log.c
 *
 */

#ifndef LOG_C_
#define LOG_C_

#include <stddef.h>
#include "util/log.h"

void (*GScale_Log)(int level, const char *file, int line, const char *function, const char *format, ...) = NULL;

#endif /* TYPES_H_ */
