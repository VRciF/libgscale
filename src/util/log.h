/*
 * log.h
 *
 *  Created on: Nov 24, 2010
 */

#ifndef LOG_H_
#define LOG_H_

#define GSCALE_LOG_LEVEL_DEBUG  1
#define GSCALE_LOG_LEVEL_INFO   2
#define GSCALE_LOG_LEVEL_WARN   4
#define GSCALE_LOG_LEVEL_ERR    8
#define GSCALE_LOG_LEVEL_FATAL 16

extern void (*GScale_Log)(int level, const char *file, int line, const char *function, const char *format, ...);

#define GSCALE_DEBUG(...) {if(GScale_Log!=NULL){ GScale_Log(GSCALE_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__); }}
#define GSCALE_INFO(...) {if(GScale_Log!=NULL){ GScale_Log(GSCALE_LOG_LEVEL_INFO, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__); }}
#define GSCALE_WARN(...) {if(GScale_Log!=NULL){ GScale_Log(GSCALE_LOG_LEVEL_WARN, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__); }}
#define GSCALE_ERR(...) {if(GScale_Log!=NULL){ GScale_Log(GSCALE_LOG_LEVEL_ERR, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__); }}
#define GSCALE_FATAL(...) {if(GScale_Log!=NULL){ GScale_Log(GSCALE_LOG_LEVEL_FATAL, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__); }}

#endif /* TYPES_H_ */
