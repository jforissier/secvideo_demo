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
#ifndef SECVIDEO_DEMO_TA_H
#define SECVIDEO_DEMO_TA_H

#define TA_SECVIDEO_DEMO_UUID { 0xffa39702, 0x9ce0, 0x47e0, \
		{ 0xa1, 0xcb, 0x40, 0x48, 0xcf, 0xdb, 0x84, 0x7d} }

/* The commands implemented in this TA */
enum {
	/*
	 * Fill the framebuffer with a solid color
	 * - params[0].value.a = B:G:R color */
	TA_SECVIDEO_DEMO_CLEAR_SCREEN = 0,
	/*
	 * Update a framebuffer area
	 * - params[0].memref points to shared memory containing image data
	 * - params[1].value.a is the offset into the target framebuffer
	 * - params[1].value.b contains flags (IMAGE_START, etc.)
	 */
	TA_SECVIDEO_DEMO_IMAGE_DATA,
};

/* Image data flags */
#define IMAGE_START	1
#define IMAGE_END	2
#define IMAGE_ENCRYPTED	4

#endif /* SECVIDEO_DEMO_TA_H */
