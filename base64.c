#include "base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static char toBase64[64] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char fromBase64[128];

void initBase64() {
	for (int i = 0; i < 64; i++) {
		fromBase64[toBase64[i]] = i;
	}
}

char* encodeBase64(void* str, size_t len, size_t* base64Len) {
	uint8_t* bytes = (uint8_t*)str;
	*base64Len = ceil(4 / 3.0 * len);
	char* base64 = calloc(*base64Len + 1, sizeof(char));
	size_t i, cnt = 0;

	for (i = 0; i + 2 < len; i += 3) {
		base64[cnt++] = toBase64[bytes[i] >> 2];
		base64[cnt++] = toBase64[((bytes[i] & 3) << 4) |
			(bytes[i + 1] >> 4)];
		base64[cnt++] = toBase64[((bytes[i + 1] & 15) << 2) |
			(bytes[i + 2] >> 6)];
		base64[cnt++] = toBase64[bytes[i + 2] & 63];
	}

	if (i < len) {
		base64[cnt++] = toBase64[bytes[i] >> 2];
		if (i + 1 < len) {
			base64[cnt++] = toBase64[((bytes[i] & 3) << 4) |
				(bytes[i + 1] >> 4)];
			base64[cnt++] = toBase64[(bytes[i + 1] & 15) << 2];
		} else {
			base64[cnt++] = toBase64[(bytes[i] & 3) << 4];
		}
	}

	*base64Len = cnt;
	return base64;
}

void* decodeBase64(char* base64, size_t base64Len, size_t* retLen) {
	*retLen = ceil(3 / 4.0 * base64Len);
	uint8_t* bytes = calloc(*retLen + 1, sizeof(uint8_t));
	size_t i, cnt = 0;
	uint8_t bix1, bix2, bix3, bix4;

	for (i = 0; i + 3 < base64Len; i += 4) {
		bix1 = fromBase64[base64[i]];
		bix2 = fromBase64[base64[i + 1]];
		bix3 = fromBase64[base64[i + 2]];
		bix4 = fromBase64[base64[i + 3]];
		bytes[cnt++] = (bix1 << 2) | (bix2 >> 4);
		bytes[cnt++] = ((bix2 & 15) << 4) | (bix3 >> 2);
		bytes[cnt++] = ((bix3 & 3) << 6) | bix4;
	}

	if (i < base64Len) {
		bix1 = fromBase64[base64[i]];
		bytes[cnt] = (bix1 << 2);
		if (i + 1 < base64Len) {
			bix2 = fromBase64[base64[i + 1]];
			bytes[cnt++] |= (bix2 >> 4);
			bytes[cnt] = ((bix2 & 15) << 4);
			if (i + 2 < base64Len) {
				bix3 = fromBase64[base64[i + 2]];
				bytes[cnt++] |= (bix3 >> 2);
				bytes[cnt++] = (bix3 & 3 << 6);
			} else cnt++;
		} else cnt++;
	}

	*retLen = cnt;
	return bytes;
}

