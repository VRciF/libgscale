#all:
#	gcc -g -ggdb -g3 -ggdb3 -ansi -fstack-protector-all -Wstack-protector -fno-omit-frame-pointer -D_GNU_SOURCE -D_XOPEN_SOURCE=600 -Wall -I../../external -I../../src -o main ./main.c
#	gcc -g -ggdb -g3 -ggdb3 -ansi -std=c99 -D_GNU_SOURCE -D_XOPEN_SOURCE=600 -Wall -Wextra -Wpadded -pedantic -Os -I../../external -I../../src -o main ./main.c
#	gcc -g -ggdb -g3 -ggdb3 -Wall -Wextra -Wpadded -O0 -DGSCALE_TRACE_ON -I../../external -I../../src -o main ./main.c

CC=gcc 
CFLAGS=-Wall -Wextra -Werror -pthread -lpthread -DSQLITE_OMIT_LOAD_EXTENSION -DGSCALE_DEBUG_ENABLE -pedantic -I../../src -I../../external -I../../external/sqlite/3.7.10 -Wno-long-long -Wno-variadic-macros -g -g3 -ggdb -ggdb3
DEPS =

OBJ=../../external/sqlite/3.7.10/sqlite3.o ../../src/exception.o ../../src/error.o ../../src/debug.o ../../src/database.o ../../src/util.o ../../src/backend/berkleysockets.o ../../src/backend/berkleysockets/database.o ../../src/backend/berkleysockets/netspeed.o ../../src/backend/berkleysockets/socket.o ../../src/backend/berkleysockets/udp.o ../../src/backend/berkleysockets/util.o ../../src/backend/internal.o ../../src/backend/loopback.o ../../src/backend/sharedmemory.o ../../src/backend/sharedmemory/util.o ../../src/datastructure/doublelinkedlist.o ../../src/datastructure/multireaderringbuffer.o ../../src/datastructure/ringbuffer.o ../../src/gscale.o ../../external/crc32/crc32.o ../../src/util/uuid.o ../../src/util/log.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
all: main.o $(OBJ)
	$(CC) -rdynamic -o main main.o $(OBJ) $(CFLAGS)

cleanall: clean
	find ../../external -name "*.o" -exec rm {} \;

clean:
	rm -f main main.o
	find ../../src -name "*.o" -exec rm {} \;
	find ../../external/crc32 -name "*.o" -exec rm {} \;
