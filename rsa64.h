#pragma once
#include <stdint.h>

void generateKeys(uint64_t* n, uint64_t* d, uint64_t* e);

uint64_t* divide(void* str, size_t len,
		int bitsPerBlock, size_t* retSize);

uint64_t* crypt(uint64_t* blocks, size_t size,
		uint64_t n, uint64_t e);

void* merge(uint64_t* blocks, size_t size,
		int bitsPerBlock, size_t* retLen);

