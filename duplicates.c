//  CITS2002 Project 2 2021
//  Name(s):             Cormac Sharkey, Avery Warddhana
//  Student number(s):   22983427, 22984998

//  Project must be compiled with `make`

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define HASH_DIGEST_SZ 32

typedef struct file_list_node_t {
	struct file_list_node_t* next;
	char* path;
	// A hard link is defined by pointing to the same inode number.
	// We'll need this to count sizes when hard-links are considered.
	ino_t inode_number;
	size_t size;
} file_list;

typedef struct hashmap_entry_t {
	unsigned char original_hash[HASH_DIGEST_SZ];
	file_list filepaths;
	struct hashmap_entry_t* next; // Here, we use separate chaining to resolve collisions
	// Open addressing, such as Robin Hood hashing, would require some implementation of dynamic resizing
	// which would involve a relatively complex implementation
} hashmap_entry;

typedef struct duplicates_hashmap_t {
	int buckets_length;
	hashmap_entry** buckets;
} duplicates_hashmap;

void hashmap_init(duplicates_hashmap* hashmap, int initial_length) {
	hashmap->buckets_length = initial_length;
	hashmap->buckets = malloc(sizeof(hashmap_entry*) * initial_length);
}

typedef enum file_type_t {
	FT_UNKNOWN,
	FT_REGULAR,
	FT_DIRECTORY,
} file_type;

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

void recurse_directory(const char* basepath, bool ignore_dotfiles, void (*callback_func)(const char*, void*), void* parameters) {
	DIR* directory = opendir(basepath);
	
	struct dirent* cur_entry;
	while ((cur_entry = readdir(directory)) != NULL) {
		char* name = cur_entry->d_name;
		
		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			continue;
		}
		if (ignore_dotfiles && name[0] == '.') {
			continue;
		}

		size_t full_path_len = strlen(basepath) + strlen(name);
		char* full_path = malloc(full_path_len + 1);
		snprintf(full_path, full_path_len, "%s/%s", basepath, name);

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



extern int SHA2(const char *filename, unsigned char *output_digest); 

file_list* construct_file_list_node(const char* filepath) {
	file_list* node = malloc(sizeof(file_list));

	struct stat file_stat;
	stat(filepath, &file_stat);
	node->path = strdup(filepath);
	node->inode_number = file_stat.st_ino;
	node->size = file_stat.st_size;

	return node;
}

void free_file_list(file_list* head) {
	file_list* tmp;
	while (head != NULL) {
		tmp = head;

		if (head->path != NULL) {
			free(head->path);
		}

		head = head->next;
		free(tmp);
	}
}

typedef struct find_by_hash_or_filename_state_t {
	const unsigned char* target_digest;
	const char* target_filepath; // Used only in the context of find_by_filename
	file_list* head;
	file_list* tail;
} find_state;

void _find_by_hash_or_filename_callback(const char* filepath, void* parameters) {
	find_state* state = (find_state*)parameters;
	unsigned char file_digest[HASH_DIGEST_SZ];
	SHA2(filepath, file_digest);

	if (memcmp(file_digest, state->target_digest, HASH_DIGEST_SZ) != 0) {
		return;
	}

	if (state->target_filepath != NULL && strcmp(filepath, state->target_filepath) == 0) {
		return;
	}

	file_list* next = construct_file_list_node(filepath);

	if (state->head == NULL) {
		state->head = next;
	} else {
		state->tail->next = next;
	}
	state->tail = next;
}

file_list* find_by_hash(const unsigned char* digest, bool ignore_dotfiles, const char** directory_list, int directory_list_length) {
	find_state state = {
		.target_digest = digest,
		.target_filepath = NULL,
		.head = NULL,
		.tail = NULL
	};
	for (int i = 0; i < directory_list_length; i++) {
		recurse_directory(directory_list[i], ignore_dotfiles, _find_by_hash_or_filename_callback, &state);
	}
	return state.head;
}

file_list* find_by_filename(const char* filename, bool ignore_dotfiles, const char** directory_list, int directory_list_length) {
	unsigned char* digest = malloc(HASH_DIGEST_SZ);
	
	if (SHA2(filename, digest)) {
		fprintf(stderr, "Cannot open specified filename");
	}
	
	find_state state = {
		.target_digest = digest,
		.target_filepath = filename,
		.head = NULL,
		.tail = NULL
	};
	for (int i = 0; i < directory_list_length; i++) {
		recurse_directory(directory_list[i], ignore_dotfiles, _find_by_hash_or_filename_callback, &state);
	}

	return state.head;
}

extern char *optarg;
extern int optind, opterr, optopt;

int main(int argc, char* argv[]) {
	bool ignore_dotfiles = true;
	bool advanced = false;
	bool quiet = false;
	bool list_all = false;
	bool modify = false;
	char* filename;
	char hash[HASH_DIGEST_SZ];
	int opt;
	while ((opt = getopt(argc, argv, "aAf::h::lmq")) != 1) {
		switch (opt) {
			case 'a':
				ignore_dotfiles = false;
				break;
			case 'A':
				advanced = true;
				break;
			case 'q':
				quiet = true;
				break;
			case 'm':
				modify = true;
				break;
			case 'l':
				list_all = true;
				break;
			case 'f':
				filename = optarg;
				break;
			case 'h':
				//TODO
				break;
		}
	}
}
