#ifndef DEBUG_H_
#define DEBUG_H_

#include <stddef.h>

struct GScale_Trace {
    char *file;
    int line;
    char *function;
    int lerrno;
    /*Fnv32_t functionhash;*/
};

#ifndef GSCALE_TRACE_SIZE
#define GSCALE_TRACE_SIZE 30
#endif
#define GSCALE_MAXIDENTIFIER 256

extern int *GSCALESTACKSTART;
extern int *GSCALECURRENTSTACK;

#define GSCALE_START_STACKTRACE() { int gscale_stackbase = 0; GSCALESTACKSTART = (&gscale_stackbase)-sizeof(int); }
#define GSCALE_TRACE_STACK() { int gscale_currentstackpos; GSCALECURRENTSTACK = (&gscale_currentstackpos)-sizeof(int); }
#define GSCALE_GETSTACKSIZE() (GSCALESTACKSTART-GSCALECURRENTSTACK)
#define GSCALE_GETSTACKPOINTER() (GSCALESTACKSTART)
#define GSCALE_CURRENTSTACKPOINTER() (GSCALECURRENTSTACK)

extern int GSCALE_TRACE_ENABLED;

/* you can use GSCALE_TRACE like the following to print a backtrace:
 * int i=0;
 * for(i=0;i<GSCALE_TRACE_CURRENT;i++){
 *      printf("%s:%d:%s\n", GSCALE_BACKTRACE[i].file, GSCALE_BACKTRACE[i].line, GSCALE_BACKTRACE[i].function);
 * }
 *
 * but keep in mind to enable tracing by
 * #define GSCALE_TRACE_ON 1
 * and having GSCALE_TRACE_ENABLED set to 1
 * GSCALE_TRACE_ENABLED = 1;
 */
extern struct GScale_Trace GSCALE_BACKTRACE[GSCALE_TRACE_SIZE];

extern int GSCALE_TRACE_CURRENT;

#define GSCALE_TRACE_RESET() {                                                                                                  \
                       GSCALE_TRACE_CURRENT = 0;                                                                                \
                       GSCALE_TRACE_PUSH();                                                                                     \
                      }

#define GSCALE_TRACE_PUSH() {                                                                                                   \
                       GSCALE_TRACE_STACK();                                                                                    \
                       if(GSCALE_TRACE_ENABLED){                                                                                \
                         if(GSCALE_TRACE_CURRENT>=GSCALE_MAXTRACE){                                                             \
                           GSCALE_TRACE_CURRENT--;                                                                              \
                           memcpy(&GSCALE_BACKTRACE[0], &GSCALE_BACKTRACE[1], sizeof(struct GScale_Trace)*(GSCALE_MAXTRACE-1)); \
                         }                                                                                                      \
                         GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].file = (char*)__FILE__;                                         \
                         GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].line = __LINE__;                                                \
                         GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].function = (char*)__PRETTY_FUNCTION__;                          \
                         GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].lerrno = errno;                                                 \
                         /*GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].functionhash = fnv_32a_str((char*)__PRETTY_FUNCTION__, 0);*/  \
                         GSCALE_TRACE_CURRENT++;                                                                                \
                         if(GSCALE_TRACE_CURRENT>=GSCALE_MAXTRACE){ GSCALE_TRACE_CURRENT--; }                                   \
                       }                                                                                                        \
                     }
#define GSCALE_TRACE_POP() {                                                                                                    \
                       if(GSCALE_TRACE_ENABLED){                                                                                \
                         if(GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].lerrno!=errno){                                              \
                             /*GSCALE_ERR("Trace detected ERROR");*/                                                            \
                         }                                                                                                      \
                         while(GSCALE_TRACE_CURRENT>0 &&                                                                        \
                               GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].file != (char*)__FILE__ &&                                       \
                               GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].function != (char*)__PRETTY_FUNCTION__                    \
                              ){                                                                                                \
                             GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].file=GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].function=NULL;  \
                             GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].line = GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].lerrno = 0;   \
                             GSCALE_TRACE_CURRENT--;                                                                            \
                         }                                                                                                      \
                         if(GSCALE_TRACE_CURRENT>0 &&                                                                           \
                            GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].file == (char*)__FILE__ &&                                          \
                            GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].function == (char*)__PRETTY_FUNCTION__){                     \
                             GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].file=GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].function=NULL;  \
                             GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].line = GSCALE_BACKTRACE[GSCALE_TRACE_CURRENT].lerrno = 0;   \
                             GSCALE_TRACE_CURRENT--;                                                                            \
                         }                                                                                                      \
                       }                                                                                                        \
                     }
#define gscale_return(r) { GSCALE_TRACE_POP(); return (r); };
#define gscale_vreturn() { GSCALE_TRACE_POP(); return; };




#ifdef GSCALE_DEBUG_ENABLE

#define GSCALE_DEBUGP(...) {struct timeval dbgtv; gettimeofday(&dbgtv,NULL); printf("[%ld.%06ld %s:%d:%s] ", dbgtv.tv_sec, dbgtv.tv_usec, __FILE__,__LINE__,__func__); printf(__VA_ARGS__); printf("\n");}
#define GSCALE_DEBUGNP() GSCALE_DEBUGP("%s","")

#else

#define GSCALE_DEBUGP(...) {}
#define GSCALE_DEBUGNP() {}

#endif

void GScale_Debug_FillTrace(char (*strings)[GSCALE_MAXIDENTIFIER], size_t ssize);

#endif
