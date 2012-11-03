#include <stdio.h>

#include "datastructure/ringbuffer.h"
#include "datastructure/multireaderringbuffer.h"
#include "minunit.h"

int tests_run = 0;

ringBuffer buffer;
ringBufferMeta meta;
char data[6];

mrRingBufferReader *reader1,*reader2,*reader3;
mrRingBufferMeta mrmeta;

static int test_init() {
    int32_t result = 0;

    result = ringBuffer_initMetaPointer(&buffer, &meta);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);
    result = ringBuffer_initDataPointer(&buffer, sizeof(data), data);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);
    result = ringBuffer_reset(&buffer);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);

    result = mrRingBuffer_init(&mrmeta, &buffer);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);
    reader1 = mrRingBuffer_initReader(&mrmeta);
    mu_assert("error, result != GSCALE_SUCCEEDED", reader1 != NULL);
    reader2 = mrRingBuffer_initReader(&mrmeta);
    mu_assert("error, result != GSCALE_SUCCEEDED", reader2 != NULL);
    reader3 = mrRingBuffer_initReader(&mrmeta);
    mu_assert("error, result != GSCALE_SUCCEEDED", reader3 != NULL);

    return 0;
}

static int test_writeIntoBuffer() {
    int32_t result = 0;

    result = ringBuffer_write(&buffer, "ABCDE", 5);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == 5);

    return 0;
}

static int test_readFromBuffer() {
    int32_t result = 0;

    char tmpbuff[6] = { '\0' };

    result = mrRingBuffer_switchToReader(reader1);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);
    result = ringBuffer_read(&buffer, tmpbuff, 1);
    mu_assert("error, result != 1", result == 1);
    result = mrRingBuffer_switchBackToRingBuffer(reader1);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);

    result = mrRingBuffer_switchToReader(reader2);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);
    result = ringBuffer_read(&buffer, tmpbuff, 2);
    mu_assert("error, result != 2", result == 2);
    result = mrRingBuffer_switchBackToRingBuffer(reader2);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);

    result = mrRingBuffer_switchToReader(reader3);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);
    result = ringBuffer_read(&buffer, tmpbuff, 3);
    mu_assert("error, result != 3", result == 3);
    result = mrRingBuffer_switchBackToRingBuffer(reader3);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);

    result = ringBuffer_getFIONREAD(&buffer);
    mu_assert_arg("error, result != 4, '%d'", result == 4, result);
    result = ringBuffer_getFIONWRITE(&buffer);
    mu_assert_arg("error, result != 1, '%d'", result == 1, result);

    result = mrRingBuffer_switchToReader(reader2);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);
    result = ringBuffer_read(&buffer, tmpbuff, 2);
    mu_assert("error, result != 2", result == 2);
    result = mrRingBuffer_switchBackToRingBuffer(reader2);
    mu_assert("error, result != GSCALE_SUCCEEDED", result == GSCALE_SUCCEEDED);

    return 0;
}

static int all_tests() {
    mu_run_test(test_init);

    mu_run_test(test_writeIntoBuffer);
    mu_run_test(test_readFromBuffer);

    return 0;
}

int main(int argc, char **argv) {
    int result = all_tests();
    if (result == 0) {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}
