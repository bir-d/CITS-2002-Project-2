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

void recurse_directory(const char* basepath, bool ignore_dotfiles, file_handler callback_func, void* parameters);


