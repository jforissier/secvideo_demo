HOST_CROSS_COMPILE ?= ../../toolchains/gcc-linaro-aarch64-linux-gnu-4.9-2014.08_linux/bin/aarch64-linux-gnu-
CROSS_COMPILE ?= $(HOST_CROSS_COMPILE)
TEE_CLIENT ?= ../../optee_client

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -I$(TEE_CLIENT)/public -I../ta/include -I../../secfb_driver
LDLIBS = -L$(TEE_CLIENT)/out/export/lib -lteec

.PHONY: all clean

all: secvideo_demo linaro-logo-web.rgba linaro-logo-web.rgba.aes

linaro-logo-web.png:
	curl https://www.linaro.org/app/images/linaro-logo-web.png -o $@

linaro-logo-web.rgba: linaro-logo-web.png
	convert $< -gravity Center -background white -extent 800x600 $@

linaro-logo-web.rgba.aes: linaro-logo-web.rgba
	openssl aes-128-ecb -nopad -nosalt -K 000102030405060708090A0B0C0D0E0F -in $< -out $@

clean:
	rm -f secvideo_demo linaro-logo-web.rgba linaro-logo-web.rgba.aes

distclean: clean
	rm -f linaro-logo-web.png

