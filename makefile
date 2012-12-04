include makefile.inc

OBJDIR:=.lib

all:
	cd src && $(MAKE)
	cd src/backend && $(MAKE)
	# stuff for current dir
	$(CC) $(CC_FLAGS) $(INCLUDE) -Iexternal -Isrc -Iinclude .lib/*.o example/loopback/main.cpp -o example/loopback/main

.PHONY: all

clean:
	rm -f $(OBJDIR)/*.o example/loopback/main
