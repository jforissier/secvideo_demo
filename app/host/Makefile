HOST_CROSS_COMPILE ?= ../../toolchains/gcc-linaro-aarch64-linux-gnu-4.9-2014.08_linux/bin/aarch64-linux-gnu-
CROSS_COMPILE ?= $(HOST_CROSS_COMPILE)
TEEC_EXPORT ?= ../../optee_client/out/export

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -I$(TEEC_EXPORT)/include -I../ta/include
LDLIBS = -L$(TEEC_EXPORT)/lib -lteec

.PHONY: all clean

all: secvideo_demo

clean:
	rm -f secvideo_demo
