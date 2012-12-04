include ../../makefile.inc
INCLUDE=-I. -I.. -I../../external

OBJDIR=.lib

CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix ../../$(OBJDIR)/,$(notdir $(CPP_FILES:.cpp=.o)))

all: $(OBJ_FILES)

../../$(OBJDIR)/%.o: %.cpp
	$(CC) $(INCLUDE) $(CC_FLAGS) -c -o $@ $<

