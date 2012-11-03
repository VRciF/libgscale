/*
 * uuid.h
 *
 *  Created on: Oct 21, 2010
 */

#ifndef UUID_H_
#define UUID_H_

typedef char GScale_UUID[16];
typedef char GScale_StrUUID[36];

/*

 from: http://www.famkruithof.net/guid-uuid-random.html

 Take 16 random bytes (octets), put them all behind each other,
 for the description the numbering starts with
 byte 1 (most significant, first) to byte 16 (least significant, last).
 Then put in the version and variant.

 To put in the version, take the 7th byte and perform an and operation using 0x0f,
 followed by an or operation with 0x40.

 To put in the variant, take the  9th byte and perform an and operation using  0x3f,
 followed by an or operation with 0x80.

 To make the string representation, take the hexadecimal presentation of bytes 1-4
 (without 0x in front of it) let them follow by a -,
 then take bytes 5 and 6, - bytes 7 and 8, - bytes 9 and 10,
 - then followed by bytes 11-16.
 The result is something like:
 00e8da9b-9ae8-4bdd-af76-af89bed2262f

 */

void UUID_createBinary(GScale_UUID uuid);
void UUID_toString_r(const GScale_UUID uuid_binary,
        GScale_StrUUID uuid_dest);
char* UUID_toString(const GScale_UUID uuid_binary);

#endif /* UUID_H_ */
