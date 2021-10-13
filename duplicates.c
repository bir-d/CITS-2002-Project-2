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
	file_list* files;
	file_list* files_tail;
	struct hashmap_entry_t* next; // Here, we use separate chaining to resolve collisions
	// Open addressing, such as Robin Hood hashing, would require some implementation of dynamic resizing
	// which would involve a relatively complex implementation
} hashmap_entry;

typedef struct duplicates_hashmap_t {
	unsigned int buckets_length;
	hashmap_entry** buckets;
} duplicates_hashmap;

// Put together, the above structures is analogous to the following:
// map[unique_data_hash_digest] = {
//     filepaths: [
//         {
//             path,
//             inode_number,
//             size
//         }
//     ]
// }

void hashmap_init(duplicates_hashmap* hashmap, int initial_length) {
	hashmap->buckets_length = initial_length;
	hashmap->buckets = malloc(sizeof(hashmap_entry*) * initial_length);
}

unsigned int hashmap_index_from_hash(duplicates_hashmap* map, unsigned char* hash) {
	unsigned int hash_int = 0;
	for (int i = 0; i < HASH_DIGEST_SZ; i++) {
		hash_int <<= 8;
		hash_int |= hash[i];
	}

	return hash_int % map->buckets_length;
}

void hashmap_insert(duplicates_hashmap* map, unsigned char* hash, file_list* node) {
	unsigned int index = hashmap_index_from_hash(map, hash);

	hashmap_entry* current_bucket = map->buckets[index];
	if (current_bucket == NULL) { // base case, this bucket is not occupied
		current_bucket = map->buckets[index] = malloc(sizeof(hashmap_entry));
		memcpy(current_bucket->original_hash, hash, HASH_DIGEST_SZ);
		current_bucket->files = current_bucket->files_tail = node;
		current_bucket->next = NULL;
		return;
	}

	while (true) {
		if (memcmp(hash, current_bucket->original_hash, HASH_DIGEST_SZ) == 0) {
			break;
		}
		if (current_bucket->next == NULL) {
			current_bucket = current_bucket->next = malloc(sizeof(hashmap_entry));
			memcpy(current_bucket->original_hash, hash, HASH_DIGEST_SZ);
			current_bucket->files = current_bucket->files_tail = node;
			current_bucket->next = NULL;
			return;
		}

		current_bucket = current_bucket->next;
	}

	current_bucket->files_tail = current_bucket->files_tail->next = node;
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

bool is_readable(const char* path) {
	return access(path, R_OK | F_OK) == 0;
}

typedef void (*file_handler)(const char* filepath, void* parameters);

void recurse_directory(const char* basepath, bool ignore_dotfiles, file_handler callback_func, void* parameters) {
	DIR* directory = opendir(basepath);	
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

		size_t full_path_len = strlen(basepath) + 1 + strlen(name);
		char* full_path = malloc(full_path_len + 1);
		snprintf(full_path, full_path_len + 1, "%s/%s", basepath, name);

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

void print_file_list(file_list* head, const char* separator) {
	while (head != NULL) {
		if (head->path != NULL) {
			printf("%s", head->path);
		}

		if (head->next != NULL) {
			printf("%s", separator);
		}

		head = head->next;
	}
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

typedef struct search_by_hash_or_filename_state_t {
	const unsigned char* target_digest;
	const char* target_filepath; // Used only in the context of search_by_filename
	file_list* head;
	file_list* tail;
} search_state;

void _search_by_hash_or_filename_callback(const char* filepath, search_state* parameters) {
	search_state* state = (search_state*)parameters;
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

file_list* search_by_hash(const unsigned char* digest, bool ignore_dotfiles, char** directory_list, int directory_list_length) {
	search_state state = {
		.target_digest = digest,
		.target_filepath = NULL,
		.head = NULL,
		.tail = NULL
	};
	for (int i = 0; i < directory_list_length; i++) {
		recurse_directory(directory_list[i], ignore_dotfiles, (file_handler)_search_by_hash_or_filename_callback, &state);
	}
	return state.head;
}

file_list* search_by_filename(const char* filename, bool ignore_dotfiles, char** directory_list, int directory_list_length) {
	unsigned char* digest = malloc(HASH_DIGEST_SZ);
	
	if (SHA2(filename, digest)) {
		fprintf(stderr, "Cannot open specified filename\n");
		exit(EXIT_FAILURE);
	}
	
	search_state state = {
		.target_digest = digest,
		.target_filepath = filename,
		.head = NULL,
		.tail = NULL
	};
	for (int i = 0; i < directory_list_length; i++) {
		recurse_directory(directory_list[i], ignore_dotfiles, (file_handler)_search_by_hash_or_filename_callback, &state);
	}

	return state.head;
}

void _search_all_callback(const char* filename, duplicates_hashmap* map) {
	unsigned char* digest = malloc(HASH_DIGEST_SZ);
	
	if (SHA2(filename, digest)) {
		fprintf(stderr, "Cannot open specified filename\n");
		exit(EXIT_FAILURE);
	}

	file_list* node = construct_file_list_node(filename);
	hashmap_insert(map, digest, node);
}

void search_all(duplicates_hashmap* output, bool ignore_dotfiles, char** directory_list, int directory_list_length) {
	for (int i = 0; i < directory_list_length; i++) {
		recurse_directory(directory_list[i], ignore_dotfiles, (file_handler)_search_all_callback, output);
	}
}

void deduplicate_file_list(file_list* head) {
	const char* target_path = head->path;
	head = head->next;
	while (head != NULL) {
		unlink(head->path);
		link(head->path, target_path);
		head = head->next;
	}
}

extern char *optarg;
extern int optind, opterr, optopt;

unsigned char* parse_hexstring(const char* hexstring, size_t expected_length) {
	// One byte -> two hex
	if (strlen(hexstring) != expected_length * 2) {
		return NULL;
	}
	unsigned char* result = calloc(expected_length, sizeof(unsigned char));

	for (int i = 0; i < expected_length; i++) {
		for (int j = 0; j < 2; j++) {
			char nibble = 0;
			char digit = hexstring[(i * 2) + j];
			if (digit >= '0' && digit <= '9') {
				nibble = digit - '0';
			} else if (digit >= 'a' && digit <= 'f') {
				nibble = digit - 'a' + 10;
			} else if (digit >= 'A' && digit <= 'F') {
				nibble = digit - 'A' + 10;
			} else {
				free(result);
				return NULL;
			}

			result[i] = (result[i] << (j ? 4 : 0)) | nibble;
		}
	}

	return result;
}

int main(int argc, char* argv[]) {
	bool print_stats = true;
	bool ignore_dotfiles = true;
	bool advanced = false;
	bool quiet = false;
	bool list_all = false;
	bool modify = false;
	char* filename = NULL;
	unsigned char* hash = NULL;
	int opt;
	while ((opt = getopt(argc, argv, "aAf:h:lmq")) != -1) {
		switch (opt) {
			case 'a':
				ignore_dotfiles = false;
				print_stats = false;
				break;
			case 'A':
				advanced = true;
				break;
			case 'q':
				quiet = true;
				print_stats = false;
				break;
			case 'm':
				modify = true;
				print_stats = false;
				break;
			case 'l':
				list_all = true;
				print_stats = false;
				break;
			case 'f':
				filename = optarg;
				print_stats = false;
				break;
			case 'h':
				if ((hash = parse_hexstring(optarg, HASH_DIGEST_SZ)) == NULL) {
					fprintf(stderr, "-h parameter must be a valid SHA256 hash (64 hex digits)\n");
					return EXIT_FAILURE;
				}
				print_stats = false;
				break;
		}
	}

	if (modify && !advanced) {
		fprintf(stderr, "-m can only be used in advanced mode (specified with -A)\n");
		return EXIT_FAILURE;
	}

	char** directory_list = &argv[optind];
	int directory_list_length = argc - optind;

	if (directory_list_length == 0) {
		fprintf(stderr, "No directories were specified\n");
		return EXIT_FAILURE;
	}

	if (!advanced && directory_list_length > 1) {
		fprintf(stderr, "Multiple directories can only be specified in advanced mode (specified with -A)\n");
		return EXIT_FAILURE;
	}

	for (int i = 0; i < directory_list_length; i++) {
		if (!is_readable(directory_list[i])) {
			fprintf(stderr, "Specified directory %s is not readable (does it exist?)\n", directory_list[i]);
			return EXIT_FAILURE;
		}
		
		if (get_file_type(directory_list[i]) != FT_DIRECTORY) {
			fprintf(stderr, "Specified directory %s is not a directory\n", directory_list[i]);
			return EXIT_FAILURE;
		}
	}

	if (filename != NULL || hash != NULL) {
		file_list* result;
		if (filename != NULL) {
			result = search_by_filename(filename, ignore_dotfiles, directory_list, directory_list_length);
		} else if (hash != NULL) {
			result = search_by_hash(hash, ignore_dotfiles, directory_list, directory_list_length);
		}
		if (!quiet) {
			print_file_list(result, "\n");
			printf("\n");
		}
		if (modify) {
			deduplicate_file_list(result);
		}
		if (quiet && result != NULL) {
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}

	duplicates_hashmap map;
	hashmap_init(&map, 1024); // A "reasonable" number to work with.
	search_all(&map, ignore_dotfiles, directory_list, directory_list_length);
	
	if (quiet) {
		bool duplicates_found = false;
		for (unsigned int i = 0; i < map.buckets_length; i++) {
			if (map.buckets[i] == NULL) {
				continue;
			}

			hashmap_entry* current = map.buckets[i];
			if (current->files->next != NULL) {
				duplicates_found = true;
				if (modify) {
					deduplicate_file_list(current->files);
				}
			}
		}

		return duplicates_found ? EXIT_FAILURE : EXIT_SUCCESS;
	}

	if (print_stats) {
		unsigned int no_files = 0;
		unsigned int total_size = 0;
		unsigned int no_unique_files = 0;
		unsigned int minimum_size = 0;
		for (unsigned int i = 0; i < map.buckets_length; i++) {
			if (map.buckets[i] == NULL) {
				continue;
			}
			

			hashmap_entry* current_bucket = map.buckets[i];
			while (current_bucket != NULL) {
				no_unique_files++;
				minimum_size += current_bucket->files->size;
				
				file_list* current_file = current_bucket->files;
				while (current_file != NULL) {
					no_files++;
					if (advanced) {
						file_list* other_file = current_bucket->files;
						bool is_hard_link = false;
						while (other_file != current_file) {
							if (other_file->inode_number == current_file->inode_number) {
								is_hard_link = true;
								break;
							}
							other_file = other_file->next;
						}

						if (!is_hard_link) {
							total_size += current_file->size;
						}
					} else {
						total_size += current_file->size;
					}

					current_file = current_file->next;
				}

				current_bucket = current_bucket->next;
			}
		}
		printf("%d\n", no_files);
		printf("%d\n", total_size);
		printf("%d\n", no_unique_files);
		printf("%d\n", minimum_size);
	} else if (list_all) {
		for (unsigned int i = 0; i < map.buckets_length; i++) {
			if (map.buckets[i] == NULL) {
				continue;
			}

			hashmap_entry* current = map.buckets[i];
			while (current != NULL) {
				print_file_list(current->files, "\t");
				printf("\n");
				current = current->next;
			}
		}
	}

	for (unsigned int i = 0; i < map.buckets_length; i++) {
		if (map.buckets[i] == NULL) {
			continue;
		}

		hashmap_entry* current = map.buckets[i];
		if (current->files->next != NULL) {
			if (modify) {
				deduplicate_file_list(current->files);
			}
		}
	}
}
