# Code Line Counter

A simple C-based utility to count the number of lines of code within a directory and it's subdirectories.

To use, set the SEARCH_PATH and INFO_PATH to valid filepaths. Attached below are instructions on how to create a .info file.
Note that both SEARCH_PATH and INFO_PATH should be absolute paths.

### Info file formatting
This utility parses .info files, which should be structured like so:

.info:
```
// ext: means that the program will only look through files with these extensions
ext: c
ext: h
ext: cpp
ext: hpp

// ecl: these are directories to exclude, relative to the SEARCH_PATH
ecl: lib
```
### Running the tool from the command line
Arguments:
'''
counter -i <info-path> -s <search-path>
'''

If no arguments are provided, it will use the default paths declared at the top of the program.
