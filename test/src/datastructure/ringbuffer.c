#include "datastructure/ringbuffer.h"
#include "minunit.h"

#include <stdio.h>

int tests_run = 0;

ringBuffer *buf = NULL;

static char * test_init() {
    buf = ringBuffer_Create(5);
    mu_assert("error, buf == NULL", buf != NULL);
    return 0;
}

static char * test_fillbuffer() {
   mu_assert("error, writelen != 2", ringBuffer_write(buf, "12", 2) == 2);
   mu_assert("error, readpos != 0", buf->readpos == 0);
   mu_assert("error, writepos != 2", buf->writepos == 2);

   mu_assert("error, fionread != 2", ringBuffer_getFIONREAD(buf) == 2);
   mu_assert("error, fionwrite != 3", ringBuffer_getFIONWRITE(buf) == 3);

   mu_assert("error, writelen != 3",ringBuffer_write(buf, "345", 3) == 3);
   mu_assert("error, readpos != 0", buf->readpos == 0);
   mu_assert("error, writepos != 5", buf->writepos == 5);

   mu_assert("error, fionread != 5", ringBuffer_getFIONREAD(buf) == 5);
   mu_assert("error, fionwrite != 0", ringBuffer_getFIONWRITE(buf) == 0);

   mu_assert("error, writelen != -1", ringBuffer_write(buf, "789", 2) == -1);
   mu_assert("error, readpos != 0", buf->readpos == 0);
   mu_assert("error, writepos != 5", buf->writepos == 5);

   mu_assert("error, fionread != 5", ringBuffer_getFIONREAD(buf) == 5);
   mu_assert("error, fionwrite != 0", ringBuffer_getFIONWRITE(buf) == 0);

   return 0;
}

static char * test_readwritebuffer() {
   char readbuf[6] = {'\0'};
   mu_assert("error, readlen != 2", ringBuffer_read(buf, readbuf, 2) == 2);
   mu_assert("error, readpos != 2", buf->readpos == 2);
   mu_assert("error, writepos != 5", buf->writepos == 5);
   mu_assert("error, read != 12", memcmp(readbuf,"12", 2) == 0);

   mu_assert("error, fionread != 3", ringBuffer_getFIONREAD(buf) == 3);
   mu_assert("error, fionwrite != 2", ringBuffer_getFIONWRITE(buf) == 2);

   mu_assert("error, writelen != 1", ringBuffer_write(buf, "1", 1) == 1);
   mu_assert("error, readpos != 0", buf->readpos == 2);
   mu_assert("error, writepos != 0", buf->writepos == 0);

   mu_assert("error, fionread != 4", ringBuffer_getFIONREAD(buf) == 4);
   mu_assert("error, fionwrite != 1", ringBuffer_getFIONWRITE(buf) == 1);

   mu_assert("error, writelen != 1", ringBuffer_write(buf, "2", 1) == 1);
   mu_assert("error, readpos != 2", buf->readpos == 2);
   mu_assert("error, writepos != 1", buf->writepos == 1);

   mu_assert("error, fionread != 5", ringBuffer_getFIONREAD(buf) == 5);
   mu_assert("error, fionwrite != 0", ringBuffer_getFIONWRITE(buf) == 0);

   mu_assert("error, writelen != -1", ringBuffer_write(buf, "3", 1) == -1);

   mu_assert("error, readlen != 2", ringBuffer_read(buf, readbuf, 2) == 2);
   mu_assert("error, readpos != 4", buf->readpos == 4);
   mu_assert("error, writepos != 1", buf->writepos == 1);
   mu_assert("error, read != 34", memcmp(readbuf,"34", 2) == 0);
   mu_assert("error, fionread != 3", ringBuffer_getFIONREAD(buf) == 3);
   mu_assert("error, fionwrite != 2", ringBuffer_getFIONWRITE(buf) == 2);

   mu_assert("error, readlen != 1", ringBuffer_read(buf, readbuf, 1) == 1);
   mu_assert("error, readpos != 5", buf->readpos == 5);
   mu_assert("error, writepos != 1", buf->writepos == 1);
   mu_assert("error, read != 5", memcmp(readbuf,"5", 1) == 0);
   mu_assert("error, fionread != 2", ringBuffer_getFIONREAD(buf) == 2);
   mu_assert("error, fionwrite != 3", ringBuffer_getFIONWRITE(buf) == 3);

   mu_assert("error, readlen != 1", ringBuffer_read(buf, readbuf, 1) == 1);
   mu_assert("error, readpos != 0", buf->readpos == 0);
   mu_assert("error, writepos != 1", buf->writepos == 1);
   mu_assert("error, read != 1", memcmp(readbuf,"1", 1) == 0);
   mu_assert("error, fionread != 1", ringBuffer_getFIONREAD(buf) == 1);
   mu_assert("error, fionwrite != 4", ringBuffer_getFIONWRITE(buf) == 4);

   mu_assert("error, readlen != 1", ringBuffer_read(buf, readbuf, 1) == 1);
   mu_assert("error, readpos != 1", buf->readpos == 1);
   mu_assert("error, writepos != 1", buf->writepos == 1);
   mu_assert("error, read != 2", memcmp(readbuf,"2", 1) == 0);
   mu_assert("error, fionread != 0", ringBuffer_getFIONREAD(buf) == 0);
   mu_assert("error, fionwrite != 5", ringBuffer_getFIONWRITE(buf) == 5);

   mu_assert("error, readlen != -1", ringBuffer_read(buf, readbuf, 1) == -1);

   return 0;
}


static char * all_tests() {
    mu_run_test(test_init);

    mu_run_test(test_fillbuffer);
    mu_run_test(test_readwritebuffer);

    return 0;
}

int main(int argc, char **argv) {
    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}

