#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
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
    do {
        
        char absPath[512];
        strcpy(absPath,dir);
        strcat(absPath,findFileData.cFileName);
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        // It's a directory
            // Skip current and parent directory ("." and "..")
            if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0) {
                continue;
            }

            // search subdirectories until recursionDepth is 0
            if (recursionDepth > 0) {
                strcat(absPath, "\\");
                lineCount += getLineCountOfFilesInDirectory(info, absPath, recursionDepth - 1);
            }
        } else {
        // It's a file
            const char* ext = get_filename_ext(absPath);
            // only read files that have a certain extension
            for (uint16_t i = 0; i < info->searchExtensionsCount; ++i) {

                if (strcmp(ext, info->searchExtensions[i])==0) {
                    lineCount += getLineCountOfFile(absPath);
                    
                    break;
                }
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

    return lineCount;
}

void trimNewline(char *line) {
    size_t len = strlen(line);
    
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len--;
    }
}

void initSearchInfo(struct searchInfo* info, const char* infoFilepath) {
    FILE* fptr;
    fptr = fopen(infoFilepath, "r");

    uint16_t includeExtCount=0;

    char line[64];
    while (fgets(line, sizeof(line), fptr) != NULL) {
        if (strlen(line) < 5)  {
            continue;
        }
        const uint8_t offset = strlen("ext: ");
        if (strncmp(line, "ext: ", offset) == 0) {
            uint8_t extLength = strlen(line) - offset;
            trimNewline(line);
            char* ext = malloc(extLength+1);
            strncpy(ext,line+offset, extLength);
            ext[extLength]='\0';
            info->searchExtensions[includeExtCount] = ext; 
            includeExtCount++;
        }
    }
    info->searchExtensionsCount = includeExtCount;
}

int main() {
    uint64_t lineCount = 0u;
    struct searchInfo info = {0,0,0,0};
    initSearchInfo(&info, "C:\\Users\\TrippR\\OneDrive\\Documents\\REPOS\\CodeLineCounter\\.info");
    const char* dir = "E:\\Graphics Programming\\Repos\\VAL\\VAL\\";

    lineCount = getLineCountOfFilesInDirectory(&info, dir, 10);
    printf("LINE COUNT: %d", lineCount);
    cleanupSearchInfo(&info);
}