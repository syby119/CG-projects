#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum IoError {
    NO_ERR = 0,
    OPEN_FILE_ERR = -1,
    READ_FILE_ERR = -100,
    OUT_OF_MEMORY = -10000,
};

enum IoError readFile(const char* filepath, char** buffer, int** offsets, int* lineCount) {
    *buffer = NULL;
    *offsets = NULL;
    *lineCount = -1;

    FILE* fp = fopen(filepath, "r");
    if (!fp) { // <---------------------------------------------- check error
        return  OPEN_FILE_ERR; // cannot open file
    }

    // get statistics of the file
    int byteCount = 0, numLines = 0;
    char lastCharacter = '\0';
    while (1) {
        int ret = fgetc(fp);
        if (ret == EOF) break; // <------------------------------ check error
        lastCharacter = (char)ret;
        if ((char)ret == '\n') ++numLines;
        ++byteCount;
    }

    if (lastCharacter != '\n' && lastCharacter != '\0') ++numLines; // add last line if needed
    byteCount += numLines; // '\0' requires extra bytes

    if (!feof(fp)) { // <---------------------------------------- check error
        fclose(fp);  // <---------------------------------------- release resources
        return READ_FILE_ERR;  // file operation error
    }
    rewind(fp);

    // allocate memory
    *buffer = (char*)malloc(byteCount * sizeof(char));
    *offsets = (int*)malloc(numLines * sizeof(int));
    *lineCount = numLines;
    if (*buffer == NULL || *offsets == NULL) {// <--------------- check error
        free(*buffer); *buffer = NULL;
        free(*offsets); *offsets = NULL;
        fclose(fp);  // <---------------------------------------- release resources
        return OUT_OF_MEMORY; // out of memory
    }

    // record data
    (*offsets)[0] = 0;
    for (int i = 0; i < numLines; ++i) {
        char* p = *buffer + (*offsets)[i];
        if (fgets(p, byteCount, fp) == NULL) {// <--------------- check error
            *lineCount = i - 1; // record valid lines
            fclose(fp); // <------------------------------------- release resources
            return READ_FILE_ERR; // file operation error
        }

        if (i < numLines) (*offsets)[i + 1] = (*offsets)[i] + strlen(p) + 1;
    }

    fclose(fp); // <--------------------------------------------- release resources
    return NO_ERR; // success
}


int main() {
    char* content = NULL;
    int* offsets = NULL;
    int numLines = 0;

    enum IoError err = readFile(__FILE__, &content, &offsets, &numLines);
    switch (err) {
        case OPEN_FILE_ERR:
            printf("cannot open file\n");
            break;
        case READ_FILE_ERR:
            printf("read file failure\n");
            break;
        case OUT_OF_MEMORY:
            printf("out of memory\n");
            break;
    }

    if (err != NO_ERR) {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numLines; ++i) {
        printf("%s", content + offsets[i]);
    }

    return 0;
}