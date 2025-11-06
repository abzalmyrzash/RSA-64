#pragma once
#include <stdbool.h>

bool printFile(const char* filename);

char* getBasename(char* path);

char* getExtension(char* filename);

#define DIR_ALREADY_EXISTS  1
#define SUCCESS             0
#define ERROR              -1

int mkdir(const char* dirName);
