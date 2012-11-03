/*
 * util.h
 *
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <sys/time.h>

#define GScale_Util_ntohll(x) ( ( (uint64_t)(ntohl( (uint32_t)((((uint64_t)x) << 32) >> 32) )) << 32) | ntohl( ((uint32_t)(((uint64_t)x) >> 32)) ) )
#define GScale_Util_htonll(x) GScale_Util_ntohll(x)

inline struct timeval
GScale_Util_Timeval_subtract (struct timeval x, struct timeval y);

#endif /* UTIL_H_ */
