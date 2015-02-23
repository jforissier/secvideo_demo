/*
 * Copyright (c) 2015, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <tee_client_api.h>
#include <secvideo_demo_ta.h>
#include <secfb_ioctl.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

#define PR(args...) do { printf(args); fflush(stdout); } while (0)

#define CHECK_INVOKE2(res, orig, fn)					    \
	do {								    \
		if (res != TEEC_SUCCESS)				    \
			errx(1, "TEEC_InvokeCommand failed with code 0x%x " \
			     "origin 0x%x", res, orig);			    \
	} while(0)

#define CHECK_INVOKE(res, orig) CHECK_INVOKE2(res, orig, "TEE_InvokeCommand")

#define CHECK(res, fn)							    \
	do {								    \
		if (res != TEEC_SUCCESS)				    \
			errx(1, fn " failed with code 0x%x ", res);	    \
	} while(0)


/* Globals */
static TEEC_Context ctx;
static TEEC_Session sess;
static TEEC_SharedMemory shm = {
	.size =  512 * 1024,
	.flags = TEEC_MEM_INPUT,
};
static TEEC_SharedMemory secm = {
	.size =  0,
	.flags = TEEC_MEM_INPUT /* remove */ | TEEC_MEM_FRAMEBUFFER,
};
static int try_read_from_secmem;

#define FP(args...) do { fprintf(stderr, args); } while(0)

static void usage()
{
	FP("Usage: secvideo_demo [-b <size>] [-c|<file>] ...\n");
	FP("       secvideo_demo -h\n");
	FP(" -b       Size of the non-secure buffer "
				"(TEEC_AllocateSharedMemory()) [%zd].\n",
				shm.size);
	FP(" -c       Clear the FVP LCD screen.\n");
	FP(" -r       Try to read back from secure memory\n");
	FP(" <file>   Display file (800x600 32-bit RGBA, A is ignored).\n");
	FP("          If extension is .aes, the file is assumed to be "
				"encrypted with 128-bit\n");
	FP("          AES-ECB, no IV, no padding, "
				"key: 0x0102030405060708090A0B0C0D0E0F.\n");
	FP(" -h       This help.\n");
}

/*
 * Fill the screen with solid RGB color
 * color[0:7]   red
 * color[8:15]  green
 * color[16:23] blue
 * color[24-31] unused
 */
static void clear_screen(uint32_t color)
{
	TEEC_Result res;
	TEEC_Operation op;
	uint32_t err_origin;

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].value.a = color;

	PR("Invoke CLEAR_SCREEN command (color=0x%08x)... \n",
	   op.params[0].value.a);
	res = TEEC_InvokeCommand(&sess, TA_SECVIDEO_DEMO_CLEAR_SCREEN, &op,
				 &err_origin);
	CHECK_INVOKE(res, err_origin);
}

static void allocate_dmabuf(viod)
{
	int ret;
	int secfb_dev;
	struct secfb_io secfb;
	void *mmaped;

	secfb_dev = open("/dev/secfb", 0);
	if (secfb_dev < 0) {
		perror("open");
		return;
	}
	ret = ioctl(secfb_dev, SECFB_IOCTL_GET_SECFB_FD, &secfb);
	if (ret < 0) {
		perror("ioctl");
		return;
	}
	PR("Note: FB size is %zd bytes\n", secfb.size);
	mmaped = mmap(NULL, secfb.size, PROT_WRITE|PROT_READ, MAP_SHARED,
			   secfb.fd, 0);
	if (!mmaped) {
		perror("mmap");
		return;
	}

#if 0 /* Need updated OP-TEE driver */
	secm.flags = TEEC_MEM_DMABUF_FD;
	secm.d.fd = secfb.fd;

	res = TEEC_RegisterSharedMemory(&ctx, &secm);
	CHECK(res, "TEEC_RegisterSharedMemory");
#endif
}

static void allocate_mem(void)
{
	TEEC_Result res;

	PR("Request shared memory (%zd bytes)...\n", shm.size);
	res = TEEC_AllocateSharedMemory(&ctx, &shm);
	CHECK(res, "TEEC_AllocateSharedMemory");
	PR("Request secure (frame buffer) memory...\n");
#if 1
	res = TEEC_AllocateSharedMemory(&ctx, &secm);
	CHECK(res, "TEEC_AllocateSharedMemory");
	PR("Note: FB size is %zd bytes\n", secm.size);
#else
	/* Not yet fully implemented */
	allocate_dmabuf();
#endif

}

static void free_mem(void)
{
	PR("Release shared memory...\n");
	TEEC_ReleaseSharedMemory(&shm);
	PR("Release secure memory...\n");
	TEEC_ReleaseSharedMemory(&secm);
}

static size_t send_image_data(size_t sz, size_t offset, int flags)
{
	TEEC_Result res;
	TEEC_Operation op;
	uint32_t err_origin;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT,
					 TEEC_VALUE_INPUT, TEEC_MEMREF_WHOLE,
					 TEEC_NONE);
	/* TA input buffer */
	op.params[0].memref.parent = &shm;
	op.params[0].memref.offset = 0;
	op.params[0].memref.size = sz;
	op.params[1].value.a = offset;
	op.params[1].value.b = flags;
	/* TA output buffer */
	op.params[2].memref.parent = &secm;

	res = TEEC_InvokeCommand(&sess, TA_SECVIDEO_DEMO_IMAGE_DATA, &op,
				 &err_origin);
	CHECK_INVOKE(res, err_origin);

	return sz;
}

static void display_file(const char *name)
{
	FILE *f;
	long file_sz;
	size_t sz, left, offset = 0;
	int crypt, flags, i;
	uint8_t *p;

	if (!shm.buffer)
		allocate_mem();

	PR("Open file '%s'\n", name);

	f = fopen(name, "r");
	if (!f) {
		perror("fopen");
		return;
	}
	fseek(f, 0, SEEK_END);
	file_sz = ftell(f);
	rewind(f);

	crypt = (strlen(name) > 4 &&
		 !strncmp(name + strlen(name) - 4, ".aes", 4));

	PR("Send image data to trusted app...\n");
	for (left = file_sz; left > 0; ) {
		sz = fread(shm.buffer, 1, shm.size, f);
		if (sz > 0) {
			PR("%zd bytes\n", sz);
			flags = 0;
			if (crypt)
				flags |= IMAGE_ENCRYPTED;
			if (left == file_sz)
				flags |= IMAGE_START;
			if (left <= sz)
				flags |= IMAGE_END;
			send_image_data(sz, offset, flags);
			left -= sz;
			offset += sz;
		}
	} while (left > 0);
	fclose(f);

	if (try_read_from_secmem) {
		p = secm.buffer;
		PR("Trying to read back from frame buffer...\n");
		for (i = 0; i < 16; i++) {
			printf("0x%02x ", p[i]);
		}
		printf("\n");
	}
}

int main(int argc, char *argv[])
{
	TEEC_Result res;
	TEEC_UUID uuid = TA_SECVIDEO_DEMO_UUID;
	uint32_t err_origin;
	int i;

	/* Show help? */
	if (argc == 1) {
		usage();
		return 0;
	}
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h")) {
			usage();
			return 0;
		}
	}

	PR("Initialize TEE context...\n");
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	PR("Open session to 'secvideo_demo' TA...\n");
	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-c")) {
			clear_screen(0x000A0000);
		} else if (!strcmp(argv[i], "-b")) {
			++i;
			shm.size = strtol(argv[i], NULL, 0);
			PR("Non-secure buffer size: %zd bytes\n", shm.size);
		} else if (!strcmp(argv[i], "-r")) {
			try_read_from_secmem = 1;
		} else {
			display_file(argv[i]);
		}
	}

	free_mem();

	PR("Close session...\n");
	TEEC_CloseSession(&sess);
	PR("Finalize context...\n");
	TEEC_FinalizeContext(&ctx);

	return 0;
}
