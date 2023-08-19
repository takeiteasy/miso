ifeq ($(OS),Windows_NT)
	PRG_SUFFIX_FLAG=1
	SOKOL_LDFLAGS := -DSOKOL_D3D11
		SOKOL_CFLAGS := -lkernel32 -luser32 -lshell32 -ldxgi -ld3d11 -lole32 -lgdi32
else
	PRG_SUFFIX_FLAG=0
	UNAME:=$(shell uname -s)
	ifeq ($(UNAME),Darwin)
		ARCH:=$(shell uname -m)
		SOKOL_LDFLAGS := -framework Cocoa -framework Metal -framework MetalKit -framework Quartz
		SOKOL_CFLAGS := -x objective-c -DSOKOL_METAL -fobjc-arc
		ifeq ($(ARCH),arm64)
			ARCH=osx_arm64
		else
			ARCH=osx
		endif
	else ifeq ($(UNAME),Linux)
		ARCH=linux
		SOKOL_LDFLAGS := -pthread -lGL -ldl -lm -lX11 -lXi -lXcursor
		SOKOL_CFLAGS := -DSOKOL_GLCORE33
	else
		$(error OS not supported by this Makefile)
	endif
endif

INCLUDES := -Isrc/ -Ideps/
LDFLAGS := $(SOKOL_LDFLAGS)
CFLAGS := $(INCLUDES) $(SOKOL_CFLAGS)

SRCS := $(wildcard examples/*.c)
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
	$(CC) $(INCLUDES) $(OBJ) $(CFLAGS) src/miso.c $(LDFLAGS) -o $(BIN)

ARCH_PATH=./tools/$(ARCH)
SHDC_PATH=$(ARCH_PATH)/sokol-shdc$(PROG_EXT)
SHADERS=$(wildcard assets/*.glsl)
SHADER_OUTS=$(patsubst %,%.h,$(SHADERS))

.SECONDEXPANSION:
SHADER=$(patsubst %.h,%,$@)
SHADER_OUT=$@
%.glsl.h: $(SHADERS)
	$(SHDC_PATH) -i $(SHADER) -o $(SHADER_OUT) -l glsl330:metal_macos:hlsl5 -b

shaders: $(SHADER_OUTS)

clean:
	rm $(OBJS) || true
	rm assets/*.air || true
	rm assets/*.dia || true
	rm assets/*.metal || true
	rm assets/*.metallib || true

veryclean: clean
	rm assets/*.glsl.h || true
	rm $(OUTS) || true

all: shaders default clean

.PHONY: default shaders clean veryclean all
