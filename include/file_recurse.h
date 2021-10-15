#pragma once

#include <stdbool.h>

typedef enum file_type_t {
	FT_UNKNOWN,
	FT_REGULAR,
	FT_DIRECTORY,
} file_type;

file_type get_file_type(const char* path);

bool is_readable(const char* path);

typedef void (*file_handler)(const char* filepath, void* parameters);

void recurse_directory(const char* dir_path, bool ignore_dotfiles, file_handler callback_func, void* parameters);
void recurse_directories(char** dir_paths, unsigned int dir_paths_len, bool ignore_dotfiles, file_handler callback_func, void* parameters);


