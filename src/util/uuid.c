/*
 * uuid.h
 *
 *  Created on: Oct 21, 2010
 */

#ifndef UUID_C_
#define UUID_C_

#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>

#include "util/uuid.h"
#include "exception.h"

/*
 * writes a 16byte binary representation of a uuid into the given uuid pointer
 * thus the pointer needs to be at min 16 bytes in size
 */
void UUID_createBinary(GScale_UUID uuid) {
    static int64_t last_tp_usec = 0;
    uint8_t i = 0;
    struct timeval tp;

    gettimeofday((struct timeval *) &tp, (NULL));
    if (tp.tv_usec != last_tp_usec) {
        srand(tp.tv_usec);
        last_tp_usec = tp.tv_usec;
    }

    for (i = 0; i < 16; i++) {
        uuid[i] = (unsigned char) rand();
    }

    uuid[6] &= 0x0f;
    uuid[6] |= 0x40;

    uuid[8] &= 0x3f;
    uuid[8] |= 0x80;
}

/*
 * the 16 bytes from uuid_binary are converted to 32 bytes hex string + 4 dashes
 * so uuid_dest must be at least 36 bytes long
 * maybe +1 for the binary 0 at the end if you want (so 37 bytes for a simple string)
 */
void UUID_toString_r(const GScale_UUID uuid_binary, GScale_StrUUID uuid_dest) {
    uint8_t uuidoffset = 0;
    uint8_t i = 0;
    char hexChar[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A',
            'B', 'C', 'D', 'E', 'F' };

    for (i = 0; i < 16; i++) {
        /* from http://answers.yahoo.com/question/index?qid=20091009023927AAaWyB4 */
        /* but its pretty easy, shift 4 bits and get hex representation */
        /* then get the hex representation of the other 4 bits */
        uuid_dest[uuidoffset] = hexChar[(uuid_binary[i] >> 4) & 0xF];
        uuid_dest[uuidoffset + 1] = hexChar[uuid_binary[i] & 0xF];

        uuidoffset += 2;

        if (uuidoffset == 8 || uuidoffset == 13 || uuidoffset == 18
                || uuidoffset == 23) {
            uuid_dest[uuidoffset] = '-';
            uuidoffset++;
        }

    }
}

char* UUID_toString(const GScale_UUID uuid_binary) {
    static char uuid_dest[sizeof(GScale_StrUUID)+1] = { '\0' };

    UUID_toString_r(uuid_binary, uuid_dest);

    return uuid_dest;
}

#endif /* UUID_H_ */
