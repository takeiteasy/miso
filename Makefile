ifeq ($(OS),Windows_NT)
	EXT=.exe
	SOKOL_FLAGS=-O2 -DSOKOL_D3D11 -lkernel32 -luser32 -lshell32 -ldxgi -ld3d11 -lole32 -lgdi32
	ARCH=win32
else
	UNAME:=$(shell uname -s)
	EXT=
	ifeq ($(UNAME),Darwin)
		SOKOL_FLAGS=-x objective-c -DSOKOL_METAL -fno-objc-arc -framework Metal -framework Cocoa -framework MetalKit -framework Quartz -framework CoreFoundation -framework CoreServices
		ARCH:=$(shell uname -m)
		ifeq ($(ARCH),arm64)
			ARCH=osx_arm64
		else
			ARCH=osx
		endif
	else ifeq ($(UNAME),Linux)
		SOKOL_FLAGS=-DSOKOL_GLCORE33 -pthread -lGL -ldl -lm -lX11 -lXi -lXcursor
		ARCH=linux
	else
		$(error OS not supported by this Makefile)
	endif
endif

default:
	$(CC) -Ideps/ $(SOKOL_FLAGS) src/*.c -o miso_$(ARCH)$(EXT)

.PHONY: default
