#include "random.h"
#include "bits.h"
#include <stdlib.h>

static int randBits;

void initRandom() {
	randBits = cntBits(RAND_MAX);
}

uint32_t random(int bits) {
	int res = 0;
	int b = 0;
	while (b <= bits) {
		res |= rand() << bits;
		b += randBits;
	}
	return res;
}

uint32_t rand32odd() {
	int r;
	do {
		r = rand();
	} while (!(r & 1));

	int res = r;
	int bits = randBits;

	while (bits <= 32) {
		res |= rand() << bits;
		bits += randBits;
	}
	return res;
}
