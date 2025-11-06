#include <stdint.h>

void initBase64();

char* encodeBase64(void* str, size_t len, size_t* base64Len);

void* decodeBase64(char* base64, size_t base64Len, size_t* retLen);
