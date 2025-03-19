/***********************************************/
#define SEARCH_PATH PLACEHOLDER
#define INFO_PATH PLACEHOLDER
/***********************************************/

/***********************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>

/***********************************************/
#define MAX_INCLUDE_EXTENSIONS 256
#define MAX_EXCLUDE_DIRECTORIES 256
/***********************************************/

void listFilesInDir(const char *directory) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;

    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", directory);

    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No files found in directory: %s\n", directory);
        return;
    }

    do {
        printf("%s\n", findFileData.cFileName);
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}


struct searchInfo {
    char* searchExtensions[MAX_INCLUDE_EXTENSIONS];
    uint16_t searchExtensionsCount;
    char* excludeDirectories[MAX_EXCLUDE_DIRECTORIES];
    uint16_t excludeDirectoryCount;
};

void cleanupSearchInfo(struct searchInfo* info) {
    for (uint16_t i = 0; i < info->searchExtensionsCount; ++i) {
        free(info->searchExtensions[i]);
    }
    for (uint16_t i = 0; i < info->excludeDirectoryCount; ++i) {
        free(info->excludeDirectories[i]);
    }
}


const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

uint32_t getLineCountOfFile(const char* filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        printf("Could not open file: %s\n", filepath);
        return 0u;
    }

    int lineCount = 0;
    int ch;
    
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            lineCount++;
        }
    }

    return lineCount;
}

uint64_t getLineCountOfFilesInDirectory(struct searchInfo* info, const char* dir, uint16_t recursionDepth) 
{
    const char* originalDir = dir;
    uint64_t lineCount = 0u;

    WIN32_FIND_DATA findFileData;
    HANDLE hFind;

    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", dir);

    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        // empty directory
        return 0;
    }

    char* tmp = malloc(256);
    strcpy(tmp,dir);
    dir = tmp;
    strcat(dir, "\\");
    do {
        
        char absPath[512];
        strcpy(absPath,dir);
        strcat(absPath,findFileData.cFileName);
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        // It's a directory
            if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0) {
                continue;
            }

            bool cont = false;
            // compare against exclude directories
            for (uint16_t i = 0; i < info->excludeDirectoryCount; ++i) {
                if (strcmp(absPath, info->excludeDirectories[i]) == 0 || strcmp(originalDir, info->excludeDirectories[i]) == 0 ) {
                    cont = true;
                    break;
                } 
            }
            if (cont) {
                continue;
            }
            if (recursionDepth > 0) {
                lineCount += getLineCountOfFilesInDirectory(info, absPath, recursionDepth - 1);
            }
        } else {
        // It's a file
            const char* ext = get_filename_ext(absPath);
            for (uint16_t i = 0; i < info->searchExtensionsCount; ++i) {
                if (strcmp(ext, info->searchExtensions[i])==0) {
                    lineCount += getLineCountOfFile(absPath);
                    
                    break;
                }
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

    free(tmp);
    return lineCount;
}

void trimNewline(char *line) {
    size_t len = strlen(line);
    
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len--;
    }
}

void initSearchInfo(struct searchInfo* info, const char* infoFilepath, const char* searchFilePath) {
    FILE* fptr;
    fptr = fopen(infoFilepath, "r");

    uint16_t includeExtCount=0u;
    uint16_t excludeDirCount=0u;

    uint16_t searchFilePathLen = strlen(searchFilePath);

    char line[64];
    while (fgets(line, sizeof(line), fptr) != NULL) {
        if (strlen(line) < 5)  {
            continue;
        }
        const uint8_t offset = strlen("ext: ");
        if (strncmp(line, "ext: ", offset) == 0) {
            uint8_t extLength = strlen(line) - offset;
            trimNewline(line);
            extLength = strlen(line) - offset;
            char* ext = malloc(extLength+1);
            strncpy(ext,line+offset, extLength);
            ext[extLength]='\0';
            info->searchExtensions[includeExtCount] = ext; 
            includeExtCount++;
        }
        if (strncmp(line, "ecl: ", offset) == 0) {
            uint8_t dirLen = strlen(line) - offset;
            trimNewline(line);
            dirLen = strlen(line) - offset;
            char* dir = malloc(dirLen+searchFilePathLen+strlen("\\")+1);
            strncpy(dir, searchFilePath, searchFilePathLen);
            strncpy(dir+searchFilePathLen, "\\", strlen("\\"));
            strncpy(dir+searchFilePathLen+strlen("\\"), line+offset, dirLen);
            
            dir[searchFilePathLen+dirLen+strlen("\\")]='\0';
            info->excludeDirectories[excludeDirCount] = dir; 
            excludeDirCount++;
        }
    }
    info->searchExtensionsCount = includeExtCount;
    info->excludeDirectoryCount = excludeDirCount;
}

int main() {
    uint64_t lineCount = 0u;
    struct searchInfo info = {0,0,0,0};
    initSearchInfo(&info, INFO_PATH, SEARCH_PATH);
    lineCount = getLineCountOfFilesInDirectory(&info, SEARCH_PATH, 10);
    printf("LINE COUNT: %d", lineCount);
    cleanupSearchInfo(&info);
}
