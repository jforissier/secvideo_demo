/**
 * Copyright (C) STM 2013. All rights reserved.
 * This code is ST-Ericsson proprietary and confidential.
 * Any use of the code for whatever purpose is subject to
 * specific written permission of STM.
 *
 */

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <secvideo_demo_ta.h>

#define STR_TRACE_USER_TA "SECVIDEO_DEMO"

#define CHECK(res, name, action) do { \
		if ((res) != TEE_SUCCESS) { \
			DMSG(name ": 0x%08x", (res)); \
			action \
		} \
	} while(0)

/* In a real world application, the secret key would be in secure storage */
static uint8_t aes_key[] =
	{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

static TEE_OperationHandle crypto_op = NULL;

/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{
}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param  params[4], void **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	DMSG("Session created");
	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */

	if (crypto_op)
		TEE_FreeOperation(crypto_op);
	DMSG("Session closed");
}

static TEE_Result clear_screen(uint32_t param_types, TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	DMSG("Clear screen request, color: 0x%08x", params[0].value.a);
	return TEE_SUCCESS;
}

static TEE_Result decrypt(void *inout, size_t sz, uint32_t keynum)
{
	TEE_Result res;
	TEE_ObjectHandle hkey;
	TEE_Attribute attr;
	size_t outsz;

	(void)keynum;
	(void)inout;
	(void)sz;

	if (!crypto_op) {
		DMSG("TEE_AllocateOperation");
		res = TEE_AllocateOperation(&crypto_op, TEE_ALG_AES_ECB_NOPAD,
					    TEE_MODE_DECRYPT, 128);
		CHECK(res, "TEE_AllocateOperation", return res;);

		DMSG("TEE_AllocateTransientObject");
		res = TEE_AllocateTransientObject(TEE_TYPE_AES, 128, &hkey);
		CHECK(res, "TEE_AllocateTransientObject", return res;);

		attr.attributeID = TEE_ATTR_SECRET_VALUE;
		attr.content.ref.buffer = aes_key;
		attr.content.ref.length = sizeof(aes_key);

		DMSG("TEE_PopulateTransientObject");
		res = TEE_PopulateTransientObject(hkey, &attr, 1);
		CHECK(res, "TEE_PopulateTransientObject", return res;);

		DMSG("TEE_SetOperationKey");
		res = TEE_SetOperationKey(crypto_op, hkey);
		CHECK(res, "TEE_SetOperationKey", return res;);

		TEE_FreeTransientObject(hkey);
	}

	DMSG("TEE_CipherInit");
	TEE_CipherInit(crypto_op, NULL, 0);

	DMSG("TEE_CipherDoFinal");
	outsz = sz;
	res = TEE_CipherDoFinal(crypto_op, inout, sz, inout, &outsz);
	CHECK(res, "TEE_CipherDoFinal", return res;);

	return TEE_SUCCESS;
}

static TEE_Result image_data(uint32_t param_types, TEE_Param params[4])
{
	TEE_Result res;
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						   TEE_PARAM_TYPE_VALUE_INPUT,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	DMSG("Image data: %zd bytes to framebuffer offset %u (encrypted: %s)",
	     params[0].memref.size, params[1].value.a,
	     params[1].value.b ? "yes" : "no");

	if (params[1].value.b) {
		res = decrypt(params[0].memref.buffer, params[0].memref.size,
				params[1].value.b);
		if (res != TEE_SUCCESS)
			return res;
	}

	return TEEExt_UpdateFrameBuffer(params[0].memref.buffer,
					params[0].memref.size,
					params[1].value.a);
}

/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */
TEE_Result TA_InvokeCommandEntryPoint(void *sess_ctx, uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4])
{
	(void)&sess_ctx; /* Unused parameter */

	switch (cmd_id) {
	case TA_SECVIDEO_DEMO_CLEAR_SCREEN:
		return clear_screen(param_types, params);
	case TA_SECVIDEO_DEMO_IMAGE_DATA:
		return image_data(param_types, params);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}
