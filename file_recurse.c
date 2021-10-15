#include <file_recurse.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

extern int SHA2(const char *filename, unsigned char *output_digest); 

file_type get_file_type(const char* path) {
	struct stat file_stat;
	stat(path, &file_stat);
	mode_t file_mode = file_stat.st_mode;

	if (S_ISREG(file_mode)) {
		return FT_REGULAR;
	}
	if (S_ISDIR(file_mode)) {
		return FT_DIRECTORY;
	}
	return FT_UNKNOWN;
}

bool is_readable(const char* path) {
	return access(path, R_OK | F_OK) == 0;
}

typedef void (*file_handler)(const char* filepath, void* parameters);

void recurse_directory(const char* dir_path, bool ignore_dotfiles, file_handler callback_func, void* parameters) {
	DIR* directory = opendir(dir_path);	
	if (directory == NULL) {
		// We have validated specified directories, passed in the command-line,
		// we can ignore descendants we don't have access to for robustness.
		return;
	}

	struct dirent* cur_entry;
	while ((cur_entry = readdir(directory)) != NULL) {
		char* name = cur_entry->d_name;
		
		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			continue;
		}
		if (ignore_dotfiles && name[0] == '.') {
			continue;
		}

		size_t full_path_len = strlen(dir_path) + 1 + strlen(name);
		char* full_path = malloc(full_path_len + 1);
		snprintf(full_path, full_path_len + 1, "%s/%s", dir_path, name);

		if (!is_readable(full_path)) {
			free(full_path);
			continue;
		}

		switch (get_file_type(full_path)) {
			case FT_REGULAR:
				callback_func(full_path, parameters);
				break;
			case FT_DIRECTORY:
				recurse_directory(full_path, ignore_dotfiles, callback_func, parameters);
				break;
			case FT_UNKNOWN:
			default:
				break;
		}
		free(full_path);
	}

	closedir(directory);
}

void recurse_directories(char** dir_paths, unsigned int dir_paths_len, bool ignore_dotfiles, file_handler callback_func, void* parameters){
	for (int i = 0; i < dir_paths_len; i++) {
		recurse_directory(dir_paths[i], ignore_dotfiles, callback_func, parameters);
	}
}
