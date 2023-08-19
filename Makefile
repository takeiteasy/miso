ifeq ($(OS),Windows_NT)
	PROG_EXT=.exe
	CFLAGS=-O2 -DSOKOL_D3D11 -lkernel32 -luser32 -lshell32 -ldxgi -ld3d11 -lole32 -lgdi32
	ARCH=win32
	SHDC_FLAGS=hlsl5
else
	UNAME:=$(shell uname -s)
	PROG_EXT=
	ifeq ($(UNAME),Darwin)
		CFLAGS=-x objective-c -DSOKOL_METAL -fobjc-arc -framework Metal -framework Cocoa -framework MetalKit -framework Quartz -framework AudioToolbox
		ARCH:=$(shell uname -m)
		ifeq ($(ARCH),arm64)
			ARCH=osx_arm64
		else
			ARCH=osx
		endif
		SHDC_FLAGS=metal_macos
	else ifeq ($(UNAME),Linux)
		CFLAGS=-DSOKOL_GLCORE33 -pthread -lGL -ldl -lm -lX11 -lasound -lXi -lXcursor
		ARCH=linux
		SHDC_FLAGS=glsl330
	else
		$(error OS not supported by this Makefile)
	endif
endif
CC=clang
SOURCE=$(wildcard src/*.c)
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
	$(CC) $(CFLAGS) -shared -fpic $(INCLUDE) $(SOURCE) -o build/libmiso_$(ARCH).dylib

web:
	emcc -DSOKOL_GLES3 $(INCLUDE) $(SOURCE) -sUSE_WEBGL2=1 -o build/miso.js

test: library
	$(CC) -Lbuild/ -lmiso_$(ARCH) $(INCLUDE) test.c -o build/test
	
default: library

all: shaders library web test cleanup

cleanup:
	rm assets/*.air
	rm assets/*.dia
	rm assets/*.metal
	rm assets/*.metallib

.PHONY: library shaders web test cleanup default all
