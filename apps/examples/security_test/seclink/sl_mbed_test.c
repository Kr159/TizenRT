/****************************************************************************
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
#include <tinyara/config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tinyara/seclink.h>
#include <tinyara/seclink_drv.h>
#include <stress_tool/st_perf.h>
#include "mbedtls/config.h"
#include "mbedtls/aes.h"
#include "mbedtls/error.h"
#include "sl_test.h"
#include "sl_test_usage.h"

static char *g_command[] = {
#ifdef SL_MBED_TEST_POOL
#undef SL_MBED_TEST_POOL
#endif
#define SL_MBED_TEST_POOL(command, type, func) command,
#include "sl_mbed_table.h"
};

#ifdef SL_MBED_TEST_POOL
#undef SL_MBED_TEST_POOL
#endif
#define SL_MBED_TEST_POOL(command, type, func) \
	extern void func(sl_options *opt);
#include "sl_mbed_table.h"

static sl_test_func g_func_list[] = {
#ifdef SL_MBED_TEST_POOL
#undef SL_MBED_TEST_POOL
#endif
#define SL_MBED_TEST_POOL(command, type, func) func,
#include "sl_mbed_table.h"
};

typedef enum {
#ifdef SL_MBED_TEST_POOL
#undef SL_MBED_TEST_POOL
#endif
#define SL_MBED_TEST_POOL(command, type, func) type,
#include "sl_mbed_table.h"
	SL_MBED_TYPE_MAX,
	SL_MBED_TYPE_ERR = -1
} sl_mbed_type_e;

ST_SET_PACK_GLOBAL(sl_mbed);
/*
 * AES test vectors from:
 *
 * http://csrc.nist.gov/archive/aes/rijndael/rijndael-vals.zip
 */
static const unsigned char aes_test_ecb_dec[3][16] = {
	{0x44, 0x41, 0x6A, 0xC2, 0xD1, 0xF5, 0x3C, 0x58,
	 0x33, 0x03, 0x91, 0x7E, 0x6B, 0xE9, 0xEB, 0xE0},
	{0x48, 0xE3, 0x1E, 0x9E, 0x25, 0x67, 0x18, 0xF2,
	 0x92, 0x29, 0x31, 0x9C, 0x19, 0xF1, 0x5B, 0xA4},
	{0x05, 0x8C, 0xCF, 0xFD, 0xBB, 0xCB, 0x38, 0x2D,
	 0x1F, 0x6F, 0x56, 0x58, 0x5D, 0x8A, 0x4A, 0xDE}
	 };

static const unsigned char aes_test_ecb_enc[3][16] = {
	{0xC3, 0x4C, 0x05, 0x2C, 0xC0, 0xDA, 0x8D, 0x73,
	 0x45, 0x1A, 0xFE, 0x5F, 0x03, 0xBE, 0x29, 0x7F},
	{0xF3, 0xF6, 0x75, 0x2A, 0xE8, 0xD7, 0x83, 0x11,
	 0x38, 0xF0, 0x41, 0x56, 0x06, 0x31, 0xB1, 0x14},
	{0x8B, 0x79, 0xEE, 0xCC, 0x93, 0xA0, 0xEE, 0x5D,
	 0xFF, 0x30, 0xB4, 0xEA, 0x21, 0x63, 0x6D, 0xA4}
	 };

#if defined(MBEDTLS_CIPHER_MODE_CBC)
static const unsigned char aes_test_cbc_dec[3][16] = {
	{0xFA, 0xCA, 0x37, 0xE0, 0xB0, 0xC8, 0x53, 0x73,
	 0xDF, 0x70, 0x6E, 0x73, 0xF7, 0xC9, 0xAF, 0x86},
	{0x5D, 0xF6, 0x78, 0xDD, 0x17, 0xBA, 0x4E, 0x75,
	 0xB6, 0x17, 0x68, 0xC6, 0xAD, 0xEF, 0x7C, 0x7B},
	{0x48, 0x04, 0xE1, 0x81, 0x8F, 0xE6, 0x29, 0x75,
	 0x19, 0xA3, 0xE8, 0x8C, 0x57, 0x31, 0x04, 0x13}
	 };

static const unsigned char aes_test_cbc_enc[3][16] = {
	{0x8A, 0x05, 0xFC, 0x5E, 0x09, 0x5A, 0xF4, 0x84,
	 0x8A, 0x08, 0xD3, 0x28, 0xD3, 0x68, 0x8E, 0x3D},
	{0x7B, 0xD9, 0x66, 0xD5, 0x3A, 0xD8, 0xC1, 0xBB,
	 0x85, 0xD2, 0xAD, 0xFA, 0xE8, 0x7B, 0xB1, 0x04},
	{0xFE, 0x3C, 0x53, 0x65, 0x3E, 0x2F, 0x45, 0xB5,
	 0x6F, 0xCD, 0x88, 0xB2, 0xCC, 0x89, 0x8F, 0xF0}
	 };
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_CFB)
/*
 * AES-CFB128 test vectors from:
 *
 * http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf
 */
static const unsigned char aes_test_cfb128_key[3][32] = {
	{0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
	 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C},
	{0x8E, 0x73, 0xB0, 0xF7, 0xDA, 0x0E, 0x64, 0x52,
	 0xC8, 0x10, 0xF3, 0x2B, 0x80, 0x90, 0x79, 0xE5,
	 0x62, 0xF8, 0xEA, 0xD2, 0x52, 0x2C, 0x6B, 0x7B},
	{0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE,
	 0x2B, 0x73, 0xAE, 0xF0, 0x85, 0x7D, 0x77, 0x81,
	 0x1F, 0x35, 0x2C, 0x07, 0x3B, 0x61, 0x08, 0xD7,
	 0x2D, 0x98, 0x10, 0xA3, 0x09, 0x14, 0xDF, 0xF4}
	 };

static const unsigned char aes_test_cfb128_iv[16] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
		};

static const unsigned char aes_test_cfb128_pt[64] = {
		0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
		0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A,
		0xAE, 0x2D, 0x8A, 0x57, 0x1E, 0x03, 0xAC, 0x9C,
		0x9E, 0xB7, 0x6F, 0xAC, 0x45, 0xAF, 0x8E, 0x51,
		0x30, 0xC8, 0x1C, 0x46, 0xA3, 0x5C, 0xE4, 0x11,
		0xE5, 0xFB, 0xC1, 0x19, 0x1A, 0x0A, 0x52, 0xEF,
		0xF6, 0x9F, 0x24, 0x45, 0xDF, 0x4F, 0x9B, 0x17,
		0xAD, 0x2B, 0x41, 0x7B, 0xE6, 0x6C, 0x37, 0x10 };

static const unsigned char aes_test_cfb128_ct[3][64] = {
		{0x3B, 0x3F, 0xD9, 0x2E, 0xB7, 0x2D, 0xAD, 0x20,
		 0x33, 0x34, 0x49, 0xF8, 0xE8, 0x3C, 0xFB, 0x4A,
		 0xC8, 0xA6, 0x45, 0x37, 0xA0, 0xB3, 0xA9, 0x3F,
		 0xCD, 0xE3, 0xCD, 0xAD, 0x9F, 0x1C, 0xE5, 0x8B,
		 0x26, 0x75, 0x1F, 0x67, 0xA3, 0xCB, 0xB1, 0x40,
		 0xB1, 0x80, 0x8C, 0xF1, 0x87, 0xA4, 0xF4, 0xDF,
		 0xC0, 0x4B, 0x05, 0x35, 0x7C, 0x5D, 0x1C, 0x0E,
		 0xEA, 0xC4, 0xC6, 0x6F, 0x9F, 0xF7, 0xF2, 0xE6},
		{0xCD, 0xC8, 0x0D, 0x6F, 0xDD, 0xF1, 0x8C, 0xAB,
		 0x34, 0xC2, 0x59, 0x09, 0xC9, 0x9A, 0x41, 0x74,
		 0x67, 0xCE, 0x7F, 0x7F, 0x81, 0x17, 0x36, 0x21,
		 0x96, 0x1A, 0x2B, 0x70, 0x17, 0x1D, 0x3D, 0x7A,
		 0x2E, 0x1E, 0x8A, 0x1D, 0xD5, 0x9B, 0x88, 0xB1,
		 0xC8, 0xE6, 0x0F, 0xED, 0x1E, 0xFA, 0xC4, 0xC9,
		 0xC0, 0x5F, 0x9F, 0x9C, 0xA9, 0x83, 0x4F, 0xA0,
		 0x42, 0xAE, 0x8F, 0xBA, 0x58, 0x4B, 0x09, 0xFF},
		{0xDC, 0x7E, 0x84, 0xBF, 0xDA, 0x79, 0x16, 0x4B,
		 0x7E, 0xCD, 0x84, 0x86, 0x98, 0x5D, 0x38, 0x60,
		 0x39, 0xFF, 0xED, 0x14, 0x3B, 0x28, 0xB1, 0xC8,
		 0x32, 0x11, 0x3C, 0x63, 0x31, 0xE5, 0x40, 0x7B,
		 0xDF, 0x10, 0x13, 0x24, 0x15, 0xE5, 0x4B, 0x92,
		 0xA1, 0x3E, 0xD0, 0xA8, 0x26, 0x7A, 0xE2, 0xF9,
		 0x75, 0xA3, 0x85, 0x74, 0x1A, 0xB9, 0xCE, 0xF8,
		 0x20, 0x31, 0x62, 0x3D, 0x55, 0xB1, 0xE4, 0x71} };
#endif /* MBEDTLS_CIPHER_MODE_CFB */

#if defined(MBEDTLS_CIPHER_MODE_CTR)
/*
 * AES-CTR test vectors from:
 *
 * http://www.faqs.org/rfcs/rfc3686.html
 */

static const unsigned char aes_test_ctr_key[3][16] = {
		{0xAE, 0x68, 0x52, 0xF8, 0x12, 0x10, 0x67, 0xCC,
		 0x4B, 0xF7, 0xA5, 0x76, 0x55, 0x77, 0xF3, 0x9E},
		{0x7E, 0x24, 0x06, 0x78, 0x17, 0xFA, 0xE0, 0xD7,
		 0x43, 0xD6, 0xCE, 0x1F, 0x32, 0x53, 0x91, 0x63},
		{0x76, 0x91, 0xBE, 0x03, 0x5E, 0x50, 0x20, 0xA8,
		 0xAC, 0x6E, 0x61, 0x85, 0x29, 0xF9, 0xA0, 0xDC} };

static const unsigned char aes_test_ctr_nonce_counter[3][16] = {
		{0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00,
		 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
		{0x00, 0x6C, 0xB6, 0xDB, 0xC0, 0x54, 0x3B, 0x59,
		 0xDA, 0x48, 0xD9, 0x0B, 0x00, 0x00, 0x00, 0x01},
		{0x00, 0xE0, 0x01, 0x7B, 0x27, 0x77, 0x7F, 0x3F,
		 0x4A, 0x17, 0x86, 0xF0, 0x00, 0x00, 0x00, 0x01} };

static const unsigned char aes_test_ctr_pt[3][48] = {
		{0x53, 0x69, 0x6E, 0x67, 0x6C, 0x65, 0x20, 0x62,
		 0x6C, 0x6F, 0x63, 0x6B, 0x20, 0x6D, 0x73, 0x67},

		{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F},

		{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		 0x20, 0x21, 0x22, 0x23} };

static const unsigned char aes_test_ctr_ct[3][48] = {
		{0xE4, 0x09, 0x5D, 0x4F, 0xB7, 0xA7, 0xB3, 0x79,
		 0x2D, 0x61, 0x75, 0xA3, 0x26, 0x13, 0x11, 0xB8},
		{0x51, 0x04, 0xA1, 0x06, 0x16, 0x8A, 0x72, 0xD9,
		 0x79, 0x0D, 0x41, 0xEE, 0x8E, 0xDA, 0xD3, 0x88,
		 0xEB, 0x2E, 0x1E, 0xFC, 0x46, 0xDA, 0x57, 0xC8,
		 0xFC, 0xE6, 0x30, 0xDF, 0x91, 0x41, 0xBE, 0x28},
		{0xC1, 0xCF, 0x48, 0xA8, 0x9F, 0x2F, 0xFD, 0xD9,
		 0xCF, 0x46, 0x52, 0xE9, 0xEF, 0xDB, 0x72, 0xD7,
		 0x45, 0x40, 0xA4, 0x2B, 0xDE, 0x6D, 0x78, 0x36,
		 0xD5, 0x9A, 0x5C, 0xEA, 0xAE, 0xF3, 0x10, 0x53,
		 0x25, 0xB2, 0x07, 0x2F} };

static const int aes_test_ctr_len[3] = {16, 32, 36};
#endif /* MBEDTLS_CIPHER_MODE_CTR */

static int mbedtls_seclink_aes_self_test(int verbose)
{
	int ret = 0, i, j, u, mode;
	unsigned int keybits;
	unsigned char key[32];
	unsigned char buf[64];
	const unsigned char *aes_tests;
#if defined(MBEDTLS_CIPHER_MODE_CBC) || defined(MBEDTLS_CIPHER_MODE_CFB)
	unsigned char iv[16];
#endif
#if defined(MBEDTLS_CIPHER_MODE_CBC)
	unsigned char prv[16];
#endif
#if defined(MBEDTLS_CIPHER_MODE_CTR) || defined(MBEDTLS_CIPHER_MODE_CFB)
	size_t offset;
#endif
#if defined(MBEDTLS_CIPHER_MODE_CTR)
	int len;
	unsigned char nonce_counter[16];
	unsigned char stream_block[16];
#endif
	mbedtls_aes_context ctx;

	memset(key, 0, 32);
	mbedtls_aes_init(&ctx);

	/*
		 * ECB mode
		 */
	for (i = 0; i < 6; i++) {
		u = i >> 1;
		keybits = 128 + u * 64;
		mode = i & 1;

		if (verbose != 0)
			printf(" AES-ECB-%3d (%s): ", keybits,
				   (mode == MBEDTLS_AES_DECRYPT) ? "dec" : "enc");

		memset(buf, 0, 16);

		if (mode == MBEDTLS_AES_DECRYPT) {
			ret = mbedtls_aes_setkey_dec(&ctx, key, keybits);
			aes_tests = aes_test_ecb_dec[u];
		} else {
			ret = mbedtls_aes_setkey_enc(&ctx, key, keybits);
			aes_tests = aes_test_ecb_enc[u];
		}

		/*
		 * AES-192 is an optional feature that may be unavailable when
		 * there is an alternative underlying implementation i.e. when
		 * MBEDTLS_AES_ALT is defined.
		 */
		if (ret == MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED && keybits == 192) {
			printf("skipped\n");
			continue;
		} else if (ret != 0) {
			goto exit;
		}

		for (j = 0; j < 10000; j++) {
			ret = mbedtls_aes_crypt_ecb(&ctx, mode, buf, buf);
			if (ret != 0)
				goto exit;
		}

		if (memcmp(buf, aes_tests, 16) != 0) {
			ret = 1;
			goto exit;
		}

		if (verbose != 0)
			printf("passed\n");
	}

	if (verbose != 0)
		printf("\n");

#if defined(MBEDTLS_CIPHER_MODE_CBC)
	/*
		 * CBC mode
		 */
	for (i = 0; i < 6; i++) {
		u = i >> 1;
		keybits = 128 + u * 64;
		mode = i & 1;

		if (verbose != 0)
			printf(" AES-CBC-%3d (%s): ", keybits,
				   (mode == MBEDTLS_AES_DECRYPT) ? "dec" : "enc");

		memset(iv, 0, 16);
		memset(prv, 0, 16);
		memset(buf, 0, 16);

		if (mode == MBEDTLS_AES_DECRYPT) {
			ret = mbedtls_aes_setkey_dec(&ctx, key, keybits);
			aes_tests = aes_test_cbc_dec[u];
		} else {
			ret = mbedtls_aes_setkey_enc(&ctx, key, keybits);
			aes_tests = aes_test_cbc_enc[u];
		}

		/*
				 * AES-192 is an optional feature that may be unavailable when
				 * there is an alternative underlying implementation i.e. when
				 * MBEDTLS_AES_ALT is defined.
				 */
		if (ret == MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED && keybits == 192) {
			printf("skipped\n");
			continue;
		} else if (ret != 0) {
			goto exit;
		}

		for (j = 0; j < 10000; j++) {
			if (mode == MBEDTLS_AES_ENCRYPT) {
				unsigned char tmp[16];

				memcpy(tmp, prv, 16);
				memcpy(prv, buf, 16);
				memcpy(buf, tmp, 16);
			}

			ret = mbedtls_aes_crypt_cbc(&ctx, mode, 16, iv, buf, buf);
			if (ret != 0)
				goto exit;
		}

		if (memcmp(buf, aes_tests, 16) != 0) {
			ret = 1;
			goto exit;
		}

		if (verbose != 0)
			printf("passed\n");
	}

	if (verbose != 0)
		printf("\n");
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_CFB)
	/*
		 * CFB128 mode
		 */
	for (i = 0; i < 6; i++) {
		u = i >> 1;
		keybits = 128 + u * 64;
		mode = i & 1;

		if (verbose != 0)
			printf(" AES-CFB128-%3d (%s): ", keybits,
				   (mode == MBEDTLS_AES_DECRYPT) ? "dec" : "enc");

		memcpy(iv, aes_test_cfb128_iv, 16);
		memcpy(key, aes_test_cfb128_key[u], keybits / 8);

		offset = 0;
		ret = mbedtls_aes_setkey_enc(&ctx, key, keybits);
		/*
				 * AES-192 is an optional feature that may be unavailable when
				 * there is an alternative underlying implementation i.e. when
				 * MBEDTLS_AES_ALT is defined.
				 */
		if (ret == MBEDTLS_ERR_PLATFORM_FEATURE_UNSUPPORTED && keybits == 192) {
			printf("skipped\n");
			continue;
		} else if (ret != 0) {
			goto exit;
		}

		if (mode == MBEDTLS_AES_DECRYPT) {
			memcpy(buf, aes_test_cfb128_ct[u], 64);
			aes_tests = aes_test_cfb128_pt;
		} else {
			memcpy(buf, aes_test_cfb128_pt, 64);
			aes_tests = aes_test_cfb128_ct[u];
		}

		ret = mbedtls_aes_crypt_cfb128(&ctx, mode, 64, &offset, iv, buf, buf);
		if (ret != 0)
			goto exit;

		if (memcmp(buf, aes_tests, 64) != 0) {
			ret = 1;
			goto exit;
		}

		if (verbose != 0)
			printf("passed\n");
	}

	if (verbose != 0)
		printf("\n");
#endif /* MBEDTLS_CIPHER_MODE_CFB */

#if defined(MBEDTLS_CIPHER_MODE_CTR)
	/*
		 * CTR mode
		 */
	for (i = 0; i < 6; i++) {
		u = i >> 1;
		mode = i & 1;

		if (verbose != 0)
			printf(" AES-CTR-128 (%s): ",
				   (mode == MBEDTLS_AES_DECRYPT) ? "dec" : "enc");

		memcpy(nonce_counter, aes_test_ctr_nonce_counter[u], 16);
		memcpy(key, aes_test_ctr_key[u], 16);

		offset = 0;
		if ((ret = mbedtls_aes_setkey_enc(&ctx, key, 128)) != 0)
			goto exit;

		len = aes_test_ctr_len[u];

		if (mode == MBEDTLS_AES_DECRYPT) {
			memcpy(buf, aes_test_ctr_ct[u], len);
			aes_tests = aes_test_ctr_pt[u];
		} else {
			memcpy(buf, aes_test_ctr_pt[u], len);
			aes_tests = aes_test_ctr_ct[u];
		}

		ret = mbedtls_aes_crypt_ctr(&ctx, len, &offset, nonce_counter,
									stream_block, buf, buf);
		if (ret != 0)
			goto exit;

		if (memcmp(buf, aes_tests, len) != 0) {
			ret = 1;
			goto exit;
		}

		if (verbose != 0)
			printf("passed\n");
	}

	if (verbose != 0)
		printf("\n");
#endif /* MBEDTLS_CIPHER_MODE_CTR */

	ret = 0;

exit:
	if (ret != 0 && verbose != 0)
		printf("failed\n");

	mbedtls_aes_free(&ctx);

	return (ret);
}

START_TEST_F(aes_selftest)
{
	ST_ASSERT_EQ(0, mbedtls_seclink_aes_self_test(1));
}
END_TEST_F

START_TEST_F(ecdsa_selftest)
{
	ST_ASSERT_EQ(0, 0);
}
END_TEST_F

void sl_handle_mbed_aes(sl_options *opt)
{
	ST_SET_SMOKE1(sl_mbed, opt->count, 0, "mbedtls AES self test", aes_selftest);
}

void sl_handle_mbed_ecdsa(sl_options *opt)
{
	ST_SET_SMOKE1(sl_mbed, opt->count, 0, "mbedtls ECDSA self test", ecdsa_selftest);
}

void sl_handle_mbed(sl_options *opt)
{
	SL_PARSE_MESSAGE(opt, g_command, sl_mbed_type_e,
					 g_func_list, SL_MBED_TYPE_MAX, SL_MBED_TYPE_ERR);
	ST_RUN_TEST(sl_mbed);
	ST_RESULT_TEST(sl_mbed);
}
