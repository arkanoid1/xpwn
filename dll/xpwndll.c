#include <stdio.h>
#include <string.h>
#include "xpwn/libxpwn.h"
#include "xpwn/nor_files.h"
#include "xpwn/img3.h"
#include "xpwndll.h"

#define BUFFERSIZE (1024*1024)

#ifdef WIN32

char* xpwntool_get_kbag(char* fileName)
{
	char* strbuf = NULL;
	AbstractFile* inFile;
	inFile = openAbstractFile2(createAbstractFileFromFile(fopen(fileName, "rb")), NULL, NULL);
	if (inFile != NULL && inFile->type == AbstractFileTypeImg3) {
		Img3Info* i3i = (Img3Info*)(inFile->data);
		if (i3i != NULL) {
			Img3Element* kbag = i3i->kbag;
			if (kbag != NULL && kbag->data != NULL && kbag->header != NULL) {
				size_t buflen = 1 + 2 * kbag->header->dataSize;
				strbuf = (char*)HeapAlloc(GetProcessHeap(), 0, buflen);
				char* bytes = (char*)kbag->data;
				for (size_t i = 0; i <  kbag->header->dataSize; ++i) {
					snprintf(strbuf + 2 * i, 2, "%02X", (unsigned char) bytes[i]);
				}
				strbuf[buflen-1] = '\0';
				return strbuf;
			}
		}
		inFile->close(inFile);
	}
	return strbuf;
}

#endif //WIN32

int xpwntool_enc_dec(const char* srcName, const char* destName, const char* templateFileName, const char* ivStr, const char* keyStr)
{
	char* inData;
	size_t inDataSize;

	AbstractFile* templateFile = NULL;
	unsigned int* key = NULL;
	unsigned int* iv = NULL;
	int hasKey = TRUE;
	int hasIV = TRUE;

	TestByteOrder();

	if (templateFileName != NULL && strlen(templateFileName) != 0) {
		templateFile = createAbstractFileFromFile(fopen(templateFileName, "rb"));
		if(!templateFile) {
			fprintf(stderr, "error: cannot open templateFile\n");
			return 1;
		}
	}

	size_t bytes = 0;
	if (keyStr != NULL && ivStr != NULL) {
		hexToInts(keyStr, &key, &bytes);
		if (bytes == 0) {
			free(key);
			key = NULL;
		} else {
			hexToInts(ivStr, &iv, &bytes);
		}
	}

	AbstractFile* inFile;
	inFile = openAbstractFile2(createAbstractFileFromFile(fopen(srcName, "rb")), key, iv);
	if(!inFile) {
		fprintf(stderr, "error: cannot open infile\n");
		return 2;
	}

	AbstractFile* outFile = createAbstractFileFromFile(fopen(destName, "wb"));
	if(!outFile) {
		fprintf(stderr, "error: cannot open outfile\n");
		return 3;
	}


	AbstractFile* newFile;

	if(templateFile) {
		newFile = duplicateAbstractFile2(templateFile, outFile, key, iv, NULL);
		if(!newFile) {
			fprintf(stderr, "error: cannot duplicate file from provided templateFile\n");
			return 4;
		}
	} else {
		newFile = outFile;
	}
	 
	if(newFile->type == AbstractFileTypeImg3) {
		AbstractFile2* abstractFile2 = (AbstractFile2*) newFile;
		if (key != NULL) {
			abstractFile2->setKey(abstractFile2, key, iv);
		}
	}

	inDataSize = (size_t) inFile->getLength(inFile);
	inData = (char*) malloc(inDataSize);
	inFile->read(inFile, inData, inDataSize);
	inFile->close(inFile);

	newFile->write(newFile, inData, inDataSize);

    if (templateFileName != NULL && strstr(destName, "WTF.")) {
		fprintf(stderr, "Exploiting 8900 vulnerability (%s)... ;)\n", destName); fflush(stderr);
		exploit8900(newFile);
	}

    newFile->close(newFile);

	free(inData);

	if(key)
		free(key);

	if(iv)
		free(iv);

	return 0;
}
