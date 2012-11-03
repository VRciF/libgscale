/*
 * util.c
 *
 */

#ifndef UTIL_C_
#define UTIL_C_

#include "util.h"

inline struct timeval
GScale_Util_Timeval_subtract (struct timeval x, struct timeval y)
{
    struct timeval result;

    /* Perform the carry for the later subtraction by updating y. */
    if (x.tv_usec < y.tv_usec) {
      int nsec = (y.tv_usec - x.tv_usec) / 1000000 + 1;
      y.tv_usec -= 1000000 * nsec;
      y.tv_sec += nsec;
    }
    if (x.tv_usec - y.tv_usec > 1000000) {
      int nsec = (x.tv_usec - y.tv_usec) / 1000000;
      y.tv_usec += 1000000 * nsec;
      y.tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    result.tv_sec = x.tv_sec - y.tv_sec;
    result.tv_usec = x.tv_usec - y.tv_usec;

    /* Return 1 if result is negative. */
    return result;
}

#endif /* UTIL_C_ */
