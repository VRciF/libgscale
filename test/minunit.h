/* file: minunit.h */
/* some modifications to get better error messages */

#define mu_assert(message, test) do { if (!(test)) { printf( ("[%s:%d:%s] " message "\n") , __FILE__,__LINE__,__FUNCTION__); return -1; } } while (0)
#define mu_assert_arg(message, test, ...) do { if (!(test)) { printf( ("[%s:%d:%s] " message "\n") , __FILE__,__LINE__,__FUNCTION__, __VA_ARGS__); return -1; } } while (0)
#define mu_run_test(test) do { int rval = test(); tests_run++; \
                               if (rval != 0) return rval; } while (0)

extern int tests_run;

