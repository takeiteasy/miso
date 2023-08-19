ifeq ($(OS),Windows_NT)
	PROG_EXT=.exe
	SOKOL_FLAGS=-O2 -DSOKOL_D3D11 -lkernel32 -luser32 -lshell32 -ldxgi -ld3d11 -lole32 -lgdi32
	ARCH=win32
else
	UNAME:=$(shell uname -s)
	PROG_EXT=
	ifeq ($(UNAME),Darwin)
		SOKOL_FLAGS=-x objective-c -DSOKOL_METAL -fobjc-arc -framework Metal -framework Cocoa -framework MetalKit -framework Quartz -framework AudioToolbox
		ARCH:=$(shell uname -m)
		ifeq ($(ARCH),arm64)
			ARCH=osx_arm64
		else
			ARCH=osx
		endif
	else ifeq ($(UNAME),Linux)
		SOKOL_FLAGS=-DSOKOL_GLCORE33 -pthread -lGL -ldl -lm -lX11 -lasound -lXi -lXcursor
		ARCH=linux
	else
		$(error OS not supported by this Makefile)
	endif
endif

CFLAGS=
LDFLAGS=
INCLUDE=-I. -Isrc/ -Ideps/
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

library:
	$(CC) $(SOKOL_FLAGS) -shared -fpic $(INCLUDE) src/miso.c -o build/libmiso_$(ARCH).dylib

web:
	emcc -DSOKOL_GLES3 $(INCLUDE) src/miso.c -sUSE_WEBGL2=1 -o build/miso.js

test: library
	$(CC) -Lbuild/ -lmiso_$(ARCH) $(INCLUDE) examples/basic.c -o build/test$(PROG_EXT)

default: library

all: shaders library web test clean

clean:
	rm assets/*.air
	rm assets/*.dia
	rm assets/*.metal
	rm assets/*.metallib

.PHONY: library shaders web test clean default all
