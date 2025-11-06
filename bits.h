#pragma once
#include <stdint.h>

static inline int cntBits(uint64_t n) {
	int cnt = 0;
	while (n) {
		n >>= 1;
		cnt++;
	}
	return cnt;
}

static inline void encodeBigEndian64(uint64_t num, void* _str) {
	uint8_t* str = _str;
	for (int i = 0; i < 8; i++) {
		str[i] = (num >> (56 - i * 8));
	}
}

static inline uint64_t decodeBigEndian64(void* _str) {
	uint8_t* str = _str;
	uint64_t num = 0;
	for (int i = 0; i < 8; i++) {
		num |= (str[i] << (56 - i * 8));
	}
	return num;
}
