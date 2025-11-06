#include "rsa64.h"
#include "base64.h"
#include "random.h"
#include "bits.h"
#include "clipboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <windows.h>
#include <inttypes.h>

const char* keynamesFilename = "keynames.txt";

void saveKeynames();

void loadKeys();

void setKeysMenu();

#define FILENAME_SIZE 256
#define MESSAGE_SIZE  4096

char message[MESSAGE_SIZE];
char filename[FILENAME_SIZE];

#define KEYS_DIR "keys"
#define KEYS_PATH "keys/"
const int keysPathLen = sizeof(KEYS_PATH) - 1;

char keyFilenameFull[FILENAME_SIZE] = KEYS_PATH;
char* keyFilename = keyFilenameFull + keysPathLen;

char privFilenameFull[FILENAME_SIZE] = KEYS_PATH;
char* privFilename = privFilenameFull + keysPathLen;

char pubFilenameFull[FILENAME_SIZE] = KEYS_PATH;
char* pubFilename = pubFilenameFull + keysPathLen;

const char pubExt[] = ".pub";
const char privExt[] = ".prv";
const int pubExtLen = sizeof(pubExt) - 1;
const int privExtLen = sizeof(privExt) - 1;
const int maxExtLen = pubExtLen > privExtLen ?
						pubExtLen : privExtLen;
const int maxNoExtSize = FILENAME_SIZE - maxExtLen - keysPathLen;

uint64_t privN = 0;
uint64_t privD = 0;
uint64_t pubN  = 0;
uint64_t pubE  = 0;

char getOption() {
	char c = toupper(getchar());
	if (c != '\n') while (getchar() != '\n');
	return c;
}

void pressEnterToContinue() {
	printf("Press enter to continue...");
	while(getchar() != '\n');
}

const char HELP_MESSAGE[] =
	"This is a program that uses the RSA public-key\n"
	"cryptography algorithm.\n"
	"\n"
	"You need to generate a pair of keys - one public\n"
	"(.pub file) and one private (.prv file).\n"
	"\n"
	"You will share your public key with your friend,\n"
	"and obviously, keep your private key to yourself.\n"
	"\n"
	"Your friend can then use your public key to encrypt\n"
	"a message, which you (and no one else*) can decrypt\n"
	"with your private key.\n"
	"\n"
	"Similarly, you will ask for your friend's public key\n"
	"to send him encrypted messages only he* can decrypt.\n"
	"\n"
	"Make sure that you have the required files in the\n"
	"program's keys directory and have set the keys before\n"
	"encryption/decryption.\n"
	"\n"
	"* No one else ideally, however this is a toy program\n"
	"  that only generates up to 64-bit keys.\n"
	"\n"
	"  Since the original RSA paper from 1977 recommends\n"
	"  200-digit (663-bit) keys, you can imagine how easy\n"
	"  it is to crack a 64-bit key now.\n"
	"\n";

int main(int argc, char* argv[]) {
	srand(time(NULL));
	initRandom();
	initBase64();

	CreateDirectory(KEYS_DIR, NULL);

	if (ERROR_ALREADY_EXISTS == GetLastError())
	{
		loadKeys();
	}
	else {
		printf("Failed to create keys directory\n");
		return 1;
	}

	bool welcomed = false;
	
mainMenu:
	system("cls");

	if (!welcomed) {
		welcomed = true;
		printf("Welcome to Abzal's 64-bit RSA encryption program!\n");
		printf("(obviously not intended for serious use)\n");
	}

	printf("\n");
	printf("MAIN MENU\n");
	printf("\n");
	printf("Enter one of the options below:\n");
	printf("\n");

	printf("(G)enerate keys\n");
	printf("(S)et keys\n");
	printf("(E)ncrypt\n");
	printf("(D)ecrypt\n");
	printf("(H)elp\n");
	printf("(Q)uit");
	printf("\n");

	#define MAIN_OPT_GENERATE_KEYS 'G'
	#define MAIN_OPT_SET_KEYS      'S'
	#define MAIN_OPT_ENCRYPT       'E'
	#define MAIN_OPT_DECRYPT       'D'
	#define MAIN_OPT_HELP          'H'
	#define MAIN_OPT_QUIT          'Q'

	const char validMainOptions[] = "GSEDHQ";

	char mainOpt;

retryMainOpt:
	printf("> ");
	mainOpt = getOption();
	if (!strchr(validMainOptions, mainOpt)) {
		printf("Invalid option!\n");
		goto retryMainOpt;
	}
	printf("\n");

	switch(mainOpt)
	{
		case MAIN_OPT_GENERATE_KEYS: {
			system("cls");
			printf("Generating keys...\n");
			uint64_t n, d, e;
			generateKeys(&n, &d, &e);

			printf("Done!\n");
			
		retryKeyName:
			printf("Enter key name "
					"(leave blank to cancel): ");

			fgets(keyFilename, maxNoExtSize, stdin);

			const int noExtLen = strcspn(keyFilename, ".\n");
			if (noExtLen == 0)
				goto mainMenu;

			keyFilename[noExtLen] = '\0';
			strcat(keyFilename, pubExt);

			FILE* file;
			file = fopen(keyFilenameFull, "r");

			if (file) {
				fclose(file);
				printf("%s exists! ", keyFilename);
				printf("Confirm overwrite? (Y/N) ");
				char c = getOption();
				if (c != 'Y') {
					goto retryKeyName;
				}
			}

			file = fopen(keyFilenameFull, "w");

			if (!file) {
				printf("Could not open %s. Please retry.\n",
						keyFilename);
				goto retryKeyName;
			}

			fprintf(file, "%" PRIu64 "\n%" PRIu64, n, e);
			fclose(file);

			printf("Public key saved in %s\n", keyFilename);

			keyFilename[noExtLen] = '\0';
			strcat(keyFilename, privExt);

			file = fopen(keyFilenameFull, "r");

			if (file) {
				fclose(file);
				printf("%s exists! ", keyFilename);
				printf("Confirm overwrite? (y/n) ");
				char c = getOption();
				if (c != 'Y') {
					goto retryKeyName;
				}
			}

			file = fopen(keyFilenameFull, "w");

			if (!file) {
				printf("Could not open %s. Please retry.\n",
						keyFilename);
				goto retryKeyName;
			}

			fprintf(file, "%" PRIu64 "\n%" PRIu64, n, d);
			fclose(file);

			printf("Private key saved in %s\n", keyFilename);

			printf("Set %s as your default private key? (y/n) ",
					keyFilename);
			char c = getOption();
			if (c == 'Y' || c == 'y') {
				privN = n;
				privD = d;
				const int totalLen = noExtLen + privExtLen;
				memcpy(privFilename, keyFilename, totalLen + 1);
				saveKeynames();
			}

			printf("Remember: only share your public key file, "
					"NOT your private key!\n");

			pressEnterToContinue();
			break;
		}

		case MAIN_OPT_SET_KEYS:
		{
			setKeysMenu();
			break;
		}

		case MAIN_OPT_ENCRYPT:
		{
			if (pubN == 0 || pubE == 0) {
				printf("Public key not set!\n");
				goto mainMenu;
			}

			system("cls");
			printf("Your message will be encrypted using %s.\n",
					pubFilename);
			printf("If you want to encrypt a file, enter blank,\n"
					"and then enter filename.\n"
					"Enter blank twice to quit to main menu.\n\n");
			printf("Enter message: ");

			char* str;
			size_t len;
			bool isFile;
			fgets(message, MESSAGE_SIZE, stdin);

			if (message[0] == '\n') {
				isFile = true;
				FILE* file;

			retryEncryptFilename:
				printf("Enter filename: ");
				fgets(filename, FILENAME_SIZE, stdin);
				int filenameLen = strcspn(filename, "\n");

				if (filenameLen == 0) goto mainMenu;
				filename[filenameLen] = '\0';

				file = fopen(filename, "rb");
				if (!file) {
					printf("Could not open file. Please retry.\n");
					goto retryEncryptFilename;
				}

				fseek(file, 0, SEEK_END);
				long fsize = ftell(file);
				fseek(file, 0, SEEK_SET);

				if (fsize == 0) {
					printf("File is empty!\n");
					pressEnterToContinue();
					break;
				}

				len = fsize + sizeof(size_t);

				str = malloc(len);
				encodeBigEndian64(fsize, str);
				fread(str + sizeof(size_t), len, 1, file);
				fclose(file);
			}

			else {
				isFile = false;
				len = strcspn(message, "\n");
				message[len] = '\0';
				str = message;
			}

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

			pressEnterToContinue();
			break;
		}

		case MAIN_OPT_DECRYPT:
		{
			if (privN == 0 || privD == 0) {
				printf("Private key not set!\n");
				goto mainMenu;
			}

			system("cls");
			printf("Your message will be decrypted using %s.\n",
					privFilename);
			printf("If you want to decrypt a file, enter blank,\n"
					"and then enter filename.\n"
					"Enter blank twice to quit to main menu.\n\n");
			printf("Enter message: ");

			uint8_t* str;
			size_t len;
			bool isFile;
			fgets(message, MESSAGE_SIZE, stdin);

			if (message[0] == '\n') {
				isFile = true;
				FILE* file;

			retryDecryptFilename:
				printf("Enter filename: ");
				fgets(filename, FILENAME_SIZE, stdin);
				int filenameLen = strcspn(filename, "\n");

				if (filenameLen == 0) goto mainMenu;
				filename[filenameLen] = '\0';

				file = fopen(filename, "rb");
				if (!file) {
					printf("Could not open file. Please retry.\n");
					goto retryDecryptFilename;
				}

				fseek(file, 0, SEEK_END);
				long fsize = ftell(file);
				fseek(file, 0, SEEK_SET);

				if (fsize == 0) {
					printf("File is empty!\n");
					pressEnterToContinue();
					break;
				}

				len = fsize;

				str = malloc(len);
				fread(str, len, 1, file);
				fclose(file);
			}

			else {
				isFile = false;
				len = strcspn(message, "\n");
				message[len] = '\0';
				printf("Decoding from Base64...\n");
				size_t decodedLen;
				str = decodeBase64(message, len, &decodedLen);
				len = decodedLen;
			}

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
			retryDecryptSave:
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
						goto retryDecryptSave;
					}
				}

				file = fopen(filename, "wb");
				if (!file) {
					printf("Could not open %s. Please retry.\n",
							filename);
					goto retryDecryptSave;
				}

				size_t fsize = decodeBigEndian64(merged);
				
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

			pressEnterToContinue();
			break;
		}

		case MAIN_OPT_QUIT:
		{
			return 0;
		}

		case MAIN_OPT_HELP:
		{
			system("cls");
			printf(HELP_MESSAGE);
			pressEnterToContinue();
			break;
		}

		default: {
			assert(0);
		}
	}
	goto mainMenu;

	return 0;
}

void saveKeynames() {
	FILE* file = fopen(keynamesFilename, "w");
	fprintf(file, "%s\n%s", privFilename, pubFilename);
	fclose(file);
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

void setKeysMenu() {
	const char validSetKeysOptions[] = "12Q";
	#define OPT_SET_PRIVATE_KEY '1'
	#define OPT_SET_PUBLIC_KEY  '2'
	#define OPT_QUIT            'Q'

setKeysMenu:
	system("cls");
	printf("\n");
	printf("SET KEYS MENU:\n");
	printf("\n");
	printf("Enter option number to set option.\n");
	printf("Enter Q to save and go back to main menu.\n");
	printf("\n");

	printf("1. Private key file: ");
	if (*privFilename) {
		printf("%s", privFilename);
	} else {
		printf("<NULL>");
	}
	printf("\n");
	printf("2. Public key file:  ");
	if (*pubFilename) {
		printf("%s", pubFilename);
	} else {
		printf("<NULL>");
	}
	printf("\n");

retrySetKeysOpt:
	printf("> ");
	char opt = getOption();
	if (!strchr(validSetKeysOptions, opt)) {
		printf("Invalid option!\n");
		goto retrySetKeysOpt;
	}

	switch (opt)
	{
		case OPT_SET_PRIVATE_KEY:
		{
		retrySetPriv:
			printf("Private key file: ");

			fgets(keyFilename, maxNoExtSize, stdin);

			const int noExtLen = strcspn(keyFilename, ".\n");
			if (noExtLen == 0)
				goto setKeysMenu;

			keyFilename[noExtLen] = '\0';
			const int totalLen = noExtLen + privExtLen;
			strcat(keyFilename, privExt);

			FILE* file;
			file = fopen(keyFilenameFull, "r");

			if (file == NULL) {
				printf("Could not open %s. Please retry.\n",
						keyFilename);
				goto retrySetPriv;
			}

			else {
				if (2 == fscanf(file,
							"%" PRIu64 "\n%" PRIu64,
							&privN, &privD)) {
					memcpy(privFilename, keyFilename, totalLen + 1);
					fclose(file);
					printf("Private key %s successfully loaded.\n",
						keyFilename);
				}

				else {
					printf("Couldn't load key from %s. "
							"Please retry.\n", keyFilename);
					goto retrySetPriv;
					fclose(file);
				}
			}

			break;
		}

		case OPT_SET_PUBLIC_KEY:
		{
		retrySetPub:
			printf("Public key file:  ");

			fgets(keyFilename, maxNoExtSize, stdin);

			const int noExtLen = strcspn(keyFilename, ".\n");
			if (noExtLen == 0)
				goto setKeysMenu;

			keyFilename[noExtLen] = '\0';
			const int totalLen = noExtLen + pubExtLen;
			strcat(keyFilename, pubExt);

			FILE* file;
			file = fopen(keyFilenameFull, "r");

			if (file == NULL) {
				printf("Could not open %s. Please retry.\n",
						keyFilename);
				goto retrySetPub;
			}

			else {
				if (2 == fscanf(file,
							"%" PRIu64 "\n%" PRIu64,
							&pubN, &pubE)) {
					memcpy(pubFilename, keyFilename, totalLen + 1);
					fclose(file);
					printf("Public key %s successfully loaded.\n",
						keyFilename);
				}

				else {
					printf("Couldn't load key from %s. "
							"Please retry.\n", keyFilename);
					goto retrySetPub;
					fclose(file);
				}
			}

			break;
		}

		case OPT_QUIT:
		{
			saveKeynames();
			return;
		}

		default:
		{
			assert(0);
		}
	}
	goto setKeysMenu;
}
