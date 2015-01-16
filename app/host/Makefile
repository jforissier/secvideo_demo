HOST_CROSS_COMPILE ?= ../../toolchains/gcc-linaro-aarch64-linux-gnu-4.9-2014.08_linux/bin/aarch64-linux-gnu-
CROSS_COMPILE ?= $(HOST_CROSS_COMPILE)
TEEC_EXPORT ?= ../../optee_client/out/export

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -I$(TEEC_EXPORT)/include -I../ta/include
LDLIBS = -L$(TEEC_EXPORT)/lib -lteec

.PHONY: all clean

all: secvideo_demo linaro-logo-web.rgba

linaro-logo-web.png:
	curl https://www.linaro.org/app/images/linaro-logo-web.png -o $@

linaro-logo-web.rgba: linaro-logo-web.png
	convert $< -gravity Center -background white -extent 800x600 $@

clean:
	rm -f secvideo_demo linaro-logo-web.rgba

distclean: clean
	rm -f linaro-logo-web.png

