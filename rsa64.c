#include "rsa64.h"
#include "random.h"
#include "bits.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <inttypes.h>

uint64_t gcd(uint64_t a, uint64_t b) {
	if (b == 0) return a;
	return gcd(b, a % b);
}

uint64_t lcm(uint64_t a, uint64_t b) {
	return a * b / gcd(a, b);
}

// M^e % n
uint64_t powmod(uint64_t M, uint64_t e, uint64_t n) {
	__uint128_t C = 1;
	int bits = cntBits(e);
	for (int i = bits - 1; i >= 0; i--) {
		C = (C * C) % n;
		if (e & (1ULL << i)) {
			C = (C * M) % n;
		}
	}
	return C;
}

int Jacobi(uint32_t a, uint32_t b) {
	if (a == 1) return 1;
	if (a & 1) {
		int num = (a - 1) * (b - 1) / 4;
		int sign = (num & 1) ? -1 : 1;
		return Jacobi(b % a, a) * sign;
	}
	int num = (b * b - 1) / 8;
	int sign = (num & 1) ? -1 : 1;
	return Jacobi(a / 2, b) * sign;
}

_Bool primalityTest(uint64_t b, int bits) {
	const int pass = 100;
	int cnt = 0;
	while (cnt < pass) {
		uint64_t a = random(bits) % (b - 1) + 1;
		if (gcd(a, b) != 1) break;
		int J = Jacobi(a, b);
		uint64_t c = powmod(a, (b - 1) / 2, b);
		if ((J + b) % b == c) {
			cnt++;
		} else {
			break;
		}
	}
	return cnt == pass;
}

_Bool isPrime(uint32_t n) {
	if (n < 2) return 0;
	for (int i = 2; i <= sqrt(n); i++) {
		if (n % i == 0) return 0;
	}
	return 1;
}

void generateKeys(uint64_t* n, uint64_t* d, uint64_t* e) {
	uint32_t p, q;
	uint64_t phi;

	do {
		p = rand32odd();
	} while(!primalityTest(p, 32));

	do {
		q = rand32odd();
	} while(!primalityTest(q, 32));

//	printf("p = %u\n", p);
//	printf("q = %u\n", q);

	uint32_t max_p_q = p > q ? p : q;
	*n = (uint64_t)p * q;
	phi = (uint64_t)(p - 1) * (q - 1);

	do {
		*d = rand32odd();
	} while (!(*d > max_p_q && primalityTest(*d, 64)));

	uint64_t x0, x1, x2, y;
	__int128_t a0, b0, a1, b1, a2, b2;

	x0 = phi;
	a0 = 1;
	b0 = 0;
	x1 = *d;
	a1 = 0;
	b1 = 1;

	while (1) {
		y = x0 / x1;

		x2 = x0 - y * x1;
		if (x2 == 0) break;

		a2 = a0 - y * a1;
		b2 = b0 - y * b1;
		if (b2 > phi / 2 || -b2 > phi / 2) {
			printf("EXCEED\n");
		}

		x0 = x1;
		x1 = x2;
		a0 = a1;
		a1 = a2;
		b0 = b1;
		b1 = b2;
	}

	if (b2 < 0) b2 += phi;
	*e = b2;
}

uint64_t* divide(void* _str, size_t len,
		int bitsPerBlock, size_t* retSize) {
	assert(len > 0);
	uint8_t* str = _str;
	const size_t size = ceil(len * 8 / (double)bitsPerBlock);
	*retSize = size;
	uint64_t* blocks = calloc(size, sizeof(*blocks));
	size_t totalBits = 0;
	size_t byte, byteOffset, bitsToWrite;
	for (size_t i = 0; i < size; i++) {
		uint64_t block = 0;
		size_t blockOffset = 0, remaining = bitsPerBlock;

		while (remaining > 0) {
			byte = totalBits / 8;
			if (byte >= len) break;
			byteOffset = totalBits % 8;
			bitsToWrite = 8 - byteOffset;
			if (remaining < bitsToWrite)
				bitsToWrite = remaining;
			block |= ((str[byte] >> byteOffset) &
					((1ULL << bitsToWrite) - 1))
					<< blockOffset;
				
			blockOffset += bitsToWrite;
			remaining -= bitsToWrite;
			totalBits += bitsToWrite;
		}
		blocks[i] = block;
		if (byte == len) break;
	}
	if (blocks[size - 1] == 0) *retSize = size - 1;
	else *retSize = size;

	return blocks;
}

uint64_t* crypt(uint64_t* blocks, size_t size,
		uint64_t n, uint64_t e) {
	assert(size > 0);
	uint64_t* res = malloc(size * sizeof(*res));
	for (size_t i = 0; i < size; i++) {
		res[i] = powmod(blocks[i], e, n);
	}
	return res;
}

void* merge(uint64_t* blocks, size_t size,
		int bitsPerBlock, size_t* retLen) {
	assert(size > 0);
	size_t len = ceil((size * bitsPerBlock) / 8.0);
	*retLen = len;
	uint8_t* str = calloc(len + 1, sizeof(uint8_t));
	size_t byte;

	for (size_t i = 0; i < size; i++) {
		const uint64_t block = blocks[i];
		size_t bitIdx = i * bitsPerBlock;
		size_t blockOffset = 0;
		size_t remaining = bitsPerBlock;

		while (remaining > 0) {
			byte = bitIdx / 8;
			size_t byteOffset = bitIdx % 8;
			size_t bitsToWrite = 8 - byteOffset;
			if (bitsToWrite > remaining)
				bitsToWrite = remaining;

			str[byte] |= ((block >> blockOffset) &
					((1 << bitsToWrite) - 1))
						<< byteOffset;

			bitIdx += bitsToWrite;
			blockOffset += bitsToWrite;
			remaining -= bitsToWrite;
		}
	}
	*retLen = byte + 1;

	return str;
}

