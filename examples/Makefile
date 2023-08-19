ifeq ($(OS),Windows_NT)
	PRG_SUFFIX_FLAG=1
	ARCH=win32
else
	PRG_SUFFIX_FLAG=0
	UNAME:=$(shell uname -s)
	ifeq ($(UNAME),Darwin)
		ARCH:=$(shell uname -m)
		ifeq ($(ARCH),arm64)
			ARCH=osx_arm64
		else
			ARCH=osx
		endif
	else ifeq ($(UNAME),Linux)
		ARCH=linux
	else
		$(error OS not supported by this Makefile)
	endif
endif

LDFLAGS := -L../build -lmiso_$(ARCH)
CFLAGS := -I../src -I../deps

SRCS := $(wildcard *.c)
PRGS := $(patsubst %.c,%,$(SRCS))
PRG_SUFFIX=.exe
BINS := $(patsubst %,%$(PRG_SUFFIX),$(PRGS))
OBJS := $(patsubst %,%.o,$(PRGS))
ifeq ($(PRG_SUFFIX_FLAG),0)
	OUTS = $(PRGS)
else
	OUTS = $(BINS)
endif

default: $(BINS)

.SECONDEXPANSION:
OBJ = $(patsubst %$(PRG_SUFFIX),%.o,$@)
ifeq ($(PRG_SUFFIX_FLAG),0)
	BIN = $(patsubst %$(PRG_SUFFIX),%,$@)
else
	BIN = $@
endif
%$(PRG_SUFFIX): $(OBJS)
	$(CC) $(OBJ) $(LDFLAGS) -o $(BIN)

clean:
	rm $(OUTS)

all: default clean

.PHONY: default clean all
