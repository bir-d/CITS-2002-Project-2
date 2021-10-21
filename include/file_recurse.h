#pragma once

#include <stdbool.h>

typedef enum file_type_t {
	FT_UNKNOWN,
	FT_REGULAR,
	FT_DIRECTORY,
} file_type;

// Retrieve the above enum a specified path
file_type get_file_type(const char* path);

// Determine if the program can read the specified path
bool is_readable(const char* path);

// A typedef for a function pointer for recurse_directory
typedef void (*file_handler)(const char* filepath, void* parameters);

// Perform a DFS search for files under dir_path, calling callback_func for each file found.
void recurse_directory(const char* dir_path, bool ignore_dotfiles, file_handler callback_func, void* parameters);

// Perform a DFS search for a list of directories
void recurse_directories(char** dir_paths, unsigned int dir_paths_len, bool ignore_dotfiles, file_handler callback_func, void* parameters);


