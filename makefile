include makefile.inc

OBJDIR:=.lib

all:
	cd src && $(MAKE)
	cd src/backend && $(MAKE)
	cd src/backend/tcp && $(MAKE)
	cd src/util && $(MAKE)
	# stuff for current dir
	$(CC) $(CC_FLAGS) $(INCLUDE) -Iexternal -Isrc -Iinclude .lib/*.o example/loopback/main.cpp -Lexternal/lib -lrt -lboost_serialization -lboost_chrono -lboost_system -o example/loopback/main
#	$(CC) $(CC_FLAGS) $(INCLUDE) -Iexternal -Isrc -Iinclude .lib/*.o example/shm/main.cpp -Lexternal/lib -lrt -lboost_serialization -lboost_chrono -lboost_system -o example/shm/main
	$(CC) $(CC_FLAGS) $(INCLUDE) -Iexternal -Isrc -Iinclude .lib/*.o example/tcp/main.cpp -Lexternal/lib -lrt -lboost_serialization -lboost_chrono -lboost_system -o example/tcp/main

.PHONY: all

clean:
	rm -f $(OBJDIR)/*.o example/loopback/main example/shm/main example/tcp/main

# make PROG="asdf" run
run:
	LD_LIBRARY_PATH=external/lib/ example/${PROG}/main
