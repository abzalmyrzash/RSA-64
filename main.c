#include "rsa64.h"
#include "base64.h"
#include "random.h"
#include "bits.h"
#include "files.h"
#include "prompt.h"
#include "clipboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <conio.h>
#include <inttypes.h>

const char* keynamesFilename = "keynames.txt";

typedef enum {
	PRIVATE,
	PUBLIC
} KeyType;

#define NONE -1

bool loadKey(KeyType keyType);

void loadKeys();

void saveKeynames();

#define FILENAME_SIZE 256
#define COMMAND_SIZE  512
#define MAX_CMD_LEN   2    // without arguments
#define MESSAGE_SIZE  4096

#define KEYS_DIR  "keys"
#define KEYS_PATH "keys/"

#define HELP_PATH "help/"

#define PUB_EXT  ".pub"
#define PRIV_EXT ".prv"

#define CMD_GENERATE_KEYS "g"
#define CMD_SET_KEYS      "k"
#define CMD_ENCRYPT       "e"
#define CMD_ENCRYPT_MANY  "em"
#define CMD_ENCRYPT_FILE  "ef"
#define CMD_DECRYPT       "d"
#define CMD_DECRYPT_FILE  "df"
#define CMD_HELP          "h"
#define CMD_QUIT          "q"

char message[MESSAGE_SIZE];
char filename[FILENAME_SIZE];

const int keysPathLen = sizeof(KEYS_PATH) - 1;

char keyFilenameFull[FILENAME_SIZE] = KEYS_PATH;
char* keyFilename = keyFilenameFull + keysPathLen;

char privFilenameFull[FILENAME_SIZE] = KEYS_PATH;
char* privFilename = privFilenameFull + keysPathLen;

char pubFilenameFull[FILENAME_SIZE] = KEYS_PATH;
char* pubFilename = pubFilenameFull + keysPathLen;

const int pubExtLen = sizeof(PUB_EXT) - 1;
const int privExtLen = sizeof(PRIV_EXT) - 1;
const int maxExtLen = pubExtLen > privExtLen ?
						pubExtLen : privExtLen;
const int maxNoExtSize = FILENAME_SIZE - maxExtLen - keysPathLen;

uint64_t privN = 0;
uint64_t privD = 0;
uint64_t pubN  = 0;
uint64_t pubE  = 0;

int main(int argc, char* argv[]) {
	srand(time(NULL));
	initRandom();
	initBase64();

	char cmd[COMMAND_SIZE];

	int mkdirRes = mkdir(KEYS_DIR);
	if (mkdirRes == DIR_ALREADY_EXISTS)
	{
		loadKeys();
	}
	else if (mkdirRes == ERROR) {
		printf("Failed to create keys directory\n");
		return 1;
	}

	printf("Welcome to Abzal's 64-bit RSA encryption program!\n"
			"(obviously not intended for serious use)\n"
			"\n"
			"Type h to get help.\n"
			"\n");

promptCmd:
	printf("> ");

	char c;
	while ((c = getchar()) == ' ');
	ungetc(c, stdin);

	fgets(cmd, COMMAND_SIZE, stdin);
	const int totalCmdLen = strlen(cmd);
	const int cmdLen = strcspn(cmd, " \n");

	if (cmdLen > MAX_CMD_LEN) {
		printf("Too long.\n");
		goto promptCmd;
	} else if (cmdLen == 0) {
		goto promptCmd;
	}

	cmd[cmdLen] = '\0';
	char* args = cmd + cmdLen + 1;
	const int argsLen = strcspn(args, "\n");
	args[argsLen] = '\0';

	if (strcmp(cmd, CMD_GENERATE_KEYS) == 0)
	{
		printf("Generating keys...\n");
		uint64_t n, d, e;
		generateKeys(&n, &d, &e);

		printf("Done!\n");

		if (argsLen > 0 && argsLen < maxNoExtSize) {
			memcpy(keyFilename, args, argsLen + 1);
			goto skipKeyName;
		} else {
			if (argsLen >= maxNoExtSize) {
				printf("Name too long!\n");
			}
		}

	promptKeyName:
		printf("Enter key name: ");
		fgets(keyFilename, maxNoExtSize, stdin);

	skipKeyName:
		const int noExtLen = strcspn(keyFilename, ".\n");
		if (noExtLen == 0) {
			goto promptKeyName;
		}

		keyFilename[noExtLen] = '\0';
		strcat(keyFilename, PUB_EXT);

		FILE* file;
		file = fopen(keyFilenameFull, "r");

		if (file) {
			fclose(file);
			printf("%s exists! ", keyFilename);
			printf("Confirm overwrite? (Y/N) ");
			char c = getOption();
			if (c != 'Y') {
				goto promptKeyName;
			}
		}

		file = fopen(keyFilenameFull, "w");

		if (!file) {
			printf("Could not open %s. Please retry.\n",
					keyFilename);
			goto promptKeyName;
		}

		fprintf(file, "%" PRIu64 "\n%" PRIu64, n, e);
		fclose(file);

		printf("Public key saved in %s\n", keyFilename);

		keyFilename[noExtLen] = '\0';
		strcat(keyFilename, PRIV_EXT);

		file = fopen(keyFilenameFull, "r");

		if (file) {
			fclose(file);
			printf("%s exists! ", keyFilename);
			printf("Confirm overwrite? (Y/N) ");
			char c = getOption();
			if (c != 'Y') {
				goto promptCmd;
			}
		}

		file = fopen(keyFilenameFull, "w");

		if (!file) {
			printf("Could not open %s. Please retry.\n",
					keyFilename);
			goto promptCmd;
		}

		fprintf(file, "%" PRIu64 "\n%" PRIu64, n, d);
		fclose(file);

		printf("Private key saved in %s\n", keyFilename);

		printf("Set %s as your default private key? (Y/N) ",
				keyFilename);
		char c = getOption();
		if (c == 'Y' || c == 'y') {
			privN = n;
			privD = d;
			const int totalLen = noExtLen + privExtLen;
			memcpy(privFilename, keyFilename, totalLen + 1);
			saveKeynames();
			printf("Saved.\n");
		}

		printf("Remember: only share your public key file, "
				"NOT your private key!\n");

		pressAnyKeyToContinue();
	}

	else if (strcmp(cmd, CMD_SET_KEYS) == 0)
	{
		char* filename = strtok(args, " \n");
		if (filename == NULL) {
			printf("Private key: %s\n", privFilename);
			printf("Public key:  %s\n", pubFilename);
			goto promptCmd;
		}

		KeyType keyType = NONE;
		int cnt = 0;
		while (filename != NULL)
		{
			const char* ext = getExtension(filename);

			if (ext == NULL) {
				printf("No extension!\n");
				goto promptCmd;
			}

			if (strcmp(ext, PUB_EXT) == 0) {
				if (keyType != PUBLIC) {
					keyType = PUBLIC;
				} else {
					printf("Select one public and "
							"one private key!\n");
					goto promptCmd;
				}
			}

			else if (strcmp(ext, PRIV_EXT) == 0) {
				if (keyType != PRIVATE) {
					keyType = PRIVATE;
				} else {
					printf("Select one public and "
							"one private key!\n");
					goto promptCmd;
				}
			}

			else {
				printf("Wrong extension!\n");
				goto promptCmd;
			}

			strcpy(keyFilename, filename);

			loadKey(keyType);

			if (++cnt == 2) break;
			filename = strtok(NULL, " \n");
		}
	}

	else if (strcmp(cmd, CMD_ENCRYPT) == 0 ||
			 strcmp(cmd, CMD_ENCRYPT_MANY) == 0 ||
			 strcmp(cmd, CMD_ENCRYPT_FILE) == 0)
	{
		if (pubN == 0 || pubE == 0) {
			printf("Public key not set!\n");
			goto promptCmd;
		}

		char* str;
		size_t len;
		bool isFile;

		if (strcmp(cmd, CMD_ENCRYPT) == 0) {
			isFile = false;
			if (argsLen == 0) {
				printf("Please provide text!\n");
				goto promptCmd;
			}
			str = args;
			len = argsLen;
		}

		else if (strcmp(cmd, CMD_ENCRYPT_MANY) == 0) {
			isFile = false;
			char ch;
			int cnt;

			if (argsLen > 0) {
				memcpy(message, args, argsLen + 1);
				message[argsLen] = '\n';
				cnt = argsLen + 1;
			} else cnt = 0;

			while (cnt < MESSAGE_SIZE - 1) {
				ch = getchar();
				if (ch == EOF) {
					if (message[cnt - 1] == '\n') {
						message[--cnt] = '\0';
					}
					break;
				}
				message[cnt++] = ch;
			}
			str = message;
			len = cnt;
		}

		else {
			isFile = true;
			FILE* file;

			if (argsLen == 0) {
				printf("Please provide filename!\n");
				goto promptCmd;
			}
			memcpy(filename, args, argsLen + 1);

			file = fopen(filename, "rb");
			if (!file) {
				printf("Could not open file. Please retry.\n");
				goto promptCmd;
			}

			const char* basename = getBasename(filename);
			const int basenameSize = strlen(basename) + 1;

			fseek(file, 0, SEEK_END);
			long fsize = ftell(file);
			fseek(file, 0, SEEK_SET);

			if (fsize == 0) {
				printf("File is empty!\n");
				goto promptCmd;
			}

			len = fsize + basenameSize + sizeof(uint64_t);

			str = malloc(len);
			encodeBigEndian64(fsize, str);
			
			fread(str + sizeof(uint64_t), len, 1, file);
			fclose(file);

			memcpy(str + sizeof(uint64_t) + fsize,
					basename, basenameSize);
		}

		printf("Using %s.\n", pubFilename);

		const int bitsPerBlock = cntBits(pubN);
		size_t size;
		uint64_t* blocks;
		uint64_t* encrypted;
		uint8_t* merged;
		size_t mergedLen;
		char* base64;
		size_t base64Len;

		printf("Dividing into blocks...\n");
		blocks = divide(str, len, bitsPerBlock - 1, &size);
		if (isFile) free(str);

		printf("Encrypting...\n");
		encrypted = crypt(blocks, size, pubN, pubE);
		free(blocks);

		printf("Merging...\n");
		merged = merge(encrypted, size, bitsPerBlock,
				&mergedLen);
		free(encrypted);

		if (isFile) {
			FILE* file;
		retryEncryptSave:
			printf("Save as: ");
			fgets(filename, FILENAME_SIZE, stdin);
			filename[strcspn(filename, "\n")] = '\0';

			file = fopen(filename, "r");
			if (file) {
				fclose(file);
				printf("%s exists! ", filename);
				printf("Confirm overwrite? (Y/N) ");
				char c = getOption();
				if (c != 'Y') {
					goto retryEncryptSave;
				}
			}

			file = fopen(filename, "wb");
			if (!file) {
				printf("Could not open %s. Please retry.\n",
						filename);
				goto retryEncryptSave;
			}
			
			fwrite(merged, mergedLen, 1, file);
			fclose(file);
			printf("File saved.\n");

			free(merged);
		}

		else {
			printf("Encoding to Base64...\n");
			base64 = encodeBase64(merged, mergedLen, &base64Len);
			free(merged);
			printf("%s\n", base64);
			copyToClipboard((char*)base64);
			printf("Result copied to clipboard automatically.\n");
			free(base64);
		}
	}

	else if (strcmp(cmd, CMD_DECRYPT) == 0 ||
			 strcmp(cmd, CMD_DECRYPT_FILE) == 0)
	{
		if (privN == 0 || privD == 0) {
			printf("Private key not set!\n");
			goto promptCmd;
		}

		uint8_t* str;
		size_t len;
		bool isFile;

		if (strcmp(cmd, CMD_DECRYPT) == 0) {
			isFile = false;
			if (argsLen == 0) {
				printf("Please provide ciphertext!\n");
				goto promptCmd;
			}
			printf("Decoding from Base64...\n");
			size_t decodedLen;
			str = decodeBase64(args, argsLen, &decodedLen);
			len = decodedLen;
		}

		else {
			isFile = true;
			FILE* file;

			if (argsLen == 0) {
				printf("Please provide filename!\n");
				goto promptCmd;
			}
			memcpy(filename, args, argsLen + 1);

			file = fopen(filename, "rb");
			if (!file) {
				printf("Could not open file. Please retry.\n");
				goto promptCmd;
			}

			fseek(file, 0, SEEK_END);
			long fsize = ftell(file);
			fseek(file, 0, SEEK_SET);

			if (fsize == 0) {
				printf("File is empty!\n");
				goto promptCmd;
			}

			len = fsize;

			str = malloc(len);
			fread(str, len, 1, file);
			fclose(file);
		}

		printf("Using %s.\n", privFilename);

		const int bitsPerBlock = cntBits(privN);
		size_t size;
		uint64_t* blocks;
		uint64_t* decrypted;
		char* merged;
		size_t mergedLen;

		printf("Dividing into blocks...\n");
		blocks = divide(str, len, bitsPerBlock, &size);
		if (isFile) free(str);

		printf("Decrypting...\n");
		decrypted = crypt(blocks, size, privN, privD);
		free(blocks);

		printf("Merging...\n");
		merged = merge(decrypted, size, bitsPerBlock - 1,
				&mergedLen);
		free(decrypted);

		if (isFile) {
			FILE* file;
			size_t fsize = decodeBigEndian64(merged);

			char* basename = merged + sizeof(uint64_t) + fsize;
			
			printf("Decrypted filename: %s\n", basename);
			printf("Save as %s? (Y/N) ", basename);
			char opt = getOption();
			if (opt == 'Y') {
				strcpy(filename, basename);
				goto skipDecryptSavePrompt;
			}

		decryptSavePrompt:
			printf("Save as: ");
			fgets(filename, FILENAME_SIZE, stdin);
			filename[strcspn(filename, "\n")] = '\0';
		skipDecryptSavePrompt:

			file = fopen(filename, "r");
			if (file) {
				fclose(file);
				printf("%s exists! ", filename);
				printf("Confirm overwrite? (Y/N) ");
				char c = getOption();
				if (c != 'Y') {
					goto decryptSavePrompt;
				}
			}

			file = fopen(filename, "wb");
			if (!file) {
				printf("Could not open %s. Please retry.\n",
						filename);
				goto decryptSavePrompt;
			}

			fwrite(merged + sizeof(size_t), fsize, 1, file);
			fclose(file);
			printf("File saved.\n");

			free(merged);
		}

		else {
			printf("%s\n", merged);
			copyToClipboard(merged);
			printf("Result copied to clipboard automatically.\n");
			free(merged);
		}
	}

	else if (strcmp(cmd, CMD_HELP) == 0)
	{
		int pageNum = 0;

		if (argsLen > 0 && 1 == sscanf(args, "%d", &pageNum)) {
			if (pageNum == 2) goto helpPage2;
			if (pageNum == 3) goto helpPage3;
		}

		if (!printFile(HELP_PATH "1.txt")) {
			printf("Help not found!\n");
			goto promptCmd;
		}

		if (pageNum == 1) goto promptCmd;
		pressAnyKeyToContinue();

	helpPage2:
		if (!printFile(HELP_PATH "2.txt")) {
			printf("Help not found!\n");
			goto promptCmd;
		}

		if (pageNum == 2) goto promptCmd;
		pressAnyKeyToContinue();

	helpPage3:
		if (!printFile(HELP_PATH "3.txt")) {
			printf("Help not found!\n");
			goto promptCmd;
		}
	}

	else if (strcmp(cmd, CMD_QUIT) == 0)
	{
		return 0;
	}

	else {
		printf("Invalid command!\n");
		goto promptCmd;
	}

	goto promptCmd;
}

void saveKeynames() {
	FILE* file = fopen(keynamesFilename, "w");
	fprintf(file, "%s\n%s", privFilename, pubFilename);
	fclose(file);
}

bool loadKey(KeyType keyType) {
	FILE* file = fopen(keyFilenameFull, "r");

	if (!file) {
		printf("Could not open file.\n");
		return false;
	}

	if (keyType == PUBLIC) {
		if (2 == fscanf(file, "%" PRIu64 "\n%" PRIu64,
					&pubN, &pubE)) {
			strcpy(pubFilename, keyFilename);
			fclose(file);
			printf("%s successfully loaded.\n", keyFilename);
			saveKeynames();
			return true;
		}
	}
	else {
		if (2 == fscanf(file, "%" PRIu64 "\n%" PRIu64,
					&privN, &privD)) {
			strcpy(privFilename, keyFilename);
			fclose(file);
			printf("%s successfully loaded.\n", keyFilename);
			saveKeynames();
			return true;
		}
	}

	printf("Failed to load key.\n");

	return false;
}

void loadKeys() {
	FILE* kFile = fopen(keynamesFilename, "r");
	if (!kFile) return;

	if (!fgets(privFilename, FILENAME_SIZE, kFile)) return;
	privFilename[strcspn(privFilename, "\n")] = '\0';

	FILE* file = fopen(privFilenameFull, "r");
	if (file) {
		fscanf(file, "%" PRIu64 "\n%" PRIu64, &privN, &privD);
		fclose(file);
	}

	if (!fgets(pubFilename, FILENAME_SIZE, kFile)) return;
	pubFilename[strcspn(pubFilename, "\n")] = '\0';

	file = fopen(pubFilenameFull, "r");
	if (file) {
		fscanf(file, "%" PRIu64 "\n%" PRIu64, &pubN, &pubE);
		fclose(file);
	}

	fclose(kFile);
}

