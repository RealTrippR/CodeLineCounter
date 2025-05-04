/***********************************************/
//#define C_SEARCH_PATH "C:/Users/TrippR/OneDrive/Documents/REPOS/VAL/VAL"
#define C_SEARCH_PATH "C:/Users/TrippR/OneDrive/Documents/CompSci/Open-forum"
#define C_INFO_PATH "C:/Users/TrippR/OneDrive/Documents/REPOS/CodeLineCounter/.info"
/***********************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "tinydir.h"

/***********************************************/
#define MAX_INCLUDE_EXTENSIONS 256
#define MAX_EXCLUDE_DIRECTORIES 256
/***********************************************/

// returns a new string
char* replaceBackSlashWithForwardSlash(const char* str) {
    const uint32_t len = strlen(str);
    char* newStr = malloc(len + 1);
    newStr = strcpy(newStr, str);
    for (uint32_t i = 0; i < len; ++i) {
        printf("%c",str[i]);
        if (newStr[i]=='\\') {newStr[i]='/';}
    }
    printf("\n");
    return newStr;
}

void listFilesInDir(const char *directory) {
    struct tinydir_dir dir;
	if (tinydir_open(&dir, ".") == -1)
	{
		perror("Error opening file");
		goto bail;
	}

	while (dir.has_next)
	{
		struct tinydir_file file;
		if (tinydir_readfile(&dir, &file) == -1)
		{
			perror("Error getting file");
			goto bail;
		}

		printf("%s", file.name);
		if (file.is_dir)
		{
			printf("/");
		}
		printf("\n");

		if (tinydir_next(&dir) == -1)
		{
			perror("Error getting next file");
			goto bail;
		}
	}

bail:
	tinydir_close(&dir);
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


// removes newlines and carriage returns from a filepath.
void trimNewline(char *line) {
    size_t len = strlen(line);
    
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[len - 1] = '\0';
        len--;
    }
}

// removes quotes from a filepath. Returns a new string
char* trimQuotes(char *str) {
    uint8_t len = strlen(str);
    if (len < 2 || (str[0] == '\"' && str[len-1] == '\"') == false) {
        return str;
    }
    char* trimmedStr = malloc(len - 1);
    
    strncpy(trimmedStr, str + 1, len - 2);
    // Null-terminate
    trimmedStr[len - 2] = '\0';
    return trimmedStr;
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

    fclose(file);

    return lineCount;
}


uint64_t getLineCountOfFilesInDirectory(struct searchInfo* info, const char* dir, uint16_t recursionDepth) 
{
    const char* originalDir = dir;
    uint64_t lineCount = 0u;


    struct tinydir_dir tinyDir;
	if (tinydir_open(&tinyDir, dir) == -1)
	{
		perror("Error opening file");
		goto bail;
	}
    
    char searchPath[256];
    snprintf(searchPath, 256, "%s/*", dir);


    char* tmp = malloc(256);
    strcpy(tmp,dir);
    dir = tmp;
    strcat(dir, "/");
    while (tinyDir.has_next)
	{
        struct tinydir_file fileData;
		if (tinydir_readfile(&tinyDir, &fileData) == -1)
		{
			perror("Error getting file");
			goto bail;
		}

        
        char absPath[512];
        strcpy(absPath,dir);
        strcat(absPath,fileData.name);
        if (fileData.is_dir) {
        // It's a directory
            if (strcmp(fileData.name, ".") == 0 || strcmp(fileData.name, "..") == 0) {
                goto nextFile;
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
                goto nextFile;
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

    nextFile:
		if (tinydir_next(&tinyDir) == -1)
		{
			perror("Error getting next file");
			goto bail;
		}
    }
bail:
	tinydir_close(&tinyDir);

    return lineCount;
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
            strncpy(dir+searchFilePathLen, "/", strlen("/"));
            strncpy(dir+searchFilePathLen+strlen("/"), line+offset, dirLen);
            
            dir[searchFilePathLen+dirLen+strlen("/")]='\0';
            info->excludeDirectories[excludeDirCount] = dir; 
            excludeDirCount++;
        }
    }
    info->searchExtensionsCount = includeExtCount;
}

int main() {
    uint64_t lineCount = 0u;
    struct searchInfo info = {0,0,0,0};
    initSearchInfo(&info, infoPath, searchPath);
    lineCount = getLineCountOfFilesInDirectory(&info, searchPath, 10);
    //listFilesInDir(INFO_PATH);
    printf("LINE COUNT: %d", lineCount);
    cleanupSearchInfo(&info);

    return EXIT_SUCCESS;
}
