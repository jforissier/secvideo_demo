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
#include <tee_client_api.h>
#include <secvideo_demo_ta.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

#define PR(args...) do { printf(args); fflush(stdout); } while (0)

#define CHECK_INVOKE2(res, orig, fn)					    \
	do {								    \
		if (res != TEEC_SUCCESS)				    \
			errx(1, "TEEC_InvokeCommand failed with code 0x%x " \
			     "origin 0x%x", res, orig);			    \
		PR("OK\n");						    \
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
static TEEC_SharedMemory shm;


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

static void allocate_shm(void)
{
	TEEC_Result res;

	shm.size = MIN(32 * 1024, TEEC_CONFIG_SHAREDMEM_MAX_SIZE);
	shm.flags = TEEC_MEM_INPUT;

	PR("Request shared memory (%zd bytes)...\n", shm.size);
	res = TEEC_AllocateSharedMemory(&ctx, &shm);
	CHECK(res, "TEEC_AllocateSharedMemory");
}

static void free_shm(void)
{
	PR("Release shared memory...\n");
	TEEC_ReleaseSharedMemory(&shm);
}

static size_t send_image_data(void *ptr, size_t sz, size_t offset)
{
	TEEC_Result res;
	TEEC_Operation op;
	uint32_t err_origin;

	assert(shm.buffer);

	if (sz > shm.size)
		return -1;

	memcpy(shm.buffer, ptr, sz);

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT,
					 TEEC_VALUE_INPUT, TEEC_NONE,
					 TEEC_NONE);
	op.params[0].memref.parent = &shm;
	op.params[0].memref.offset = 0;
	op.params[0].memref.size = sz;
	op.params[1].value.a = offset;

	PR("Invoke IMAGE_DATA command...");
	res = TEEC_InvokeCommand(&sess, TA_SECVIDEO_DEMO_IMAGE_DATA, &op,
				 &err_origin);
	CHECK_INVOKE(res, err_origin);

	return sz;
}

static void display_image(uint8_t *buf, size_t buf_sz)
{
	uint8_t *p = buf;
	size_t s, left = buf_sz;

	while (left > 0) {
		s = send_image_data(p, MIN(left, shm.size), p - buf);
		if (s < 0)
			break;
		p += s;
		left -= s;
	}
}

static void display_file(const char *name)
{
	FILE *f;
	long file_sz;
	uint8_t *buf;
	size_t buf_sz = 800 * 600 * 4;

	buf = malloc(buf_sz);
	if (!buf)
		errx(1, "malloc failed");

	PR("Open file '%s'\n", name);

	f = fopen(name, "r");
	if (!f) {
		perror("fopen");
		return;
	}
	fseek(f, 0, SEEK_END);
	file_sz = ftell(f);
	if (file_sz != buf_sz) {
		PR("Warning: file_sz != buf_sz (%lu != %zd)\n", file_sz,
		   buf_sz);
		buf_sz = MIN(buf_sz, file_sz);
	}
	rewind(f);
	if (fread(buf, buf_sz, 1, f) != 1) {
		perror("fread");
		buf_sz = 0;
	}
	fclose(f);

	display_image(buf, buf_sz);

	free(buf);
}

int main(int argc, char *argv[])
{
	TEEC_Result res;
	TEEC_UUID uuid = TA_SECVIDEO_DEMO_UUID;
	uint32_t err_origin;
	int i;

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

	clear_screen(0x000A0000);

	allocate_shm();

	for (i = 1; i < argc; i++)
		display_file(argv[i]);

	free_shm();

	PR("Close session...\n");
	TEEC_CloseSession(&sess);
	PR("Finalize context...\n");
	TEEC_FinalizeContext(&ctx);

	return 0;
}
