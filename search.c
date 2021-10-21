#include "search.h"

void _search_by_hash_or_filename_callback(const char* filepath, search_state* parameters) {
	search_state* state = (search_state*)parameters;
	unsigned char file_digest[HASH_DIGEST_SZ];
	SHA2(filepath, file_digest);

	// Is it the file we're looking for?
	if (memcmp(file_digest, state->target_digest, HASH_DIGEST_SZ) != 0) {
		return;
	}

	// Is it the file we we specified by name?
	if (state->target_filepath != NULL && strcmp(filepath, state->target_filepath) == 0) {
		return;
	}

	file_list* next = file_list_construct_from_file(filepath);

	if (next == NULL) {
		return;
	}

	// Append to list
	if (state->head == NULL) {
		state->head = next;
	} else {
		state->tail->next = next;
	}
	state->tail = next;
}

file_list* search_by_hash(const unsigned char* digest, bool ignore_dotfiles, char** directories, int directories_length) {
	search_state state = {
		.target_digest = digest,
		.target_filepath = NULL,
		.head = NULL,
		.tail = NULL
	};
	for (int i = 0; i < directories_length; i++) {
		recurse_directory(directories[i], ignore_dotfiles, (file_handler)_search_by_hash_or_filename_callback, &state);
	}
	return state.head;
}

file_list* search_by_filename(const char* filename, bool ignore_dotfiles, char** directories, int directories_length) {
	unsigned char* digest = malloc(HASH_DIGEST_SZ);
	
	if (SHA2(filename, digest)) {
		fprintf(stderr, "Cannot open %s\n", filename);
		exit(EXIT_FAILURE);
	}
	
	search_state state = {
		.target_digest = digest,
		.target_filepath = filename,
		.head = NULL,
		.tail = NULL
	};
	recurse_directories(directories, directories_length, ignore_dotfiles, (file_handler)_search_by_hash_or_filename_callback, &state);

	free(digest);
	return state.head;
}

void _search_all_callback(const char* filename, duplicates_hashmap* map) {
	unsigned char* digest = malloc(HASH_DIGEST_SZ);
	
	if (SHA2(filename, digest)) {
		fprintf(stderr, "Cannot open %s\n", filename);
	}

	file_list* node = file_list_construct_from_file(filename);
	if (node != NULL) {
		hashmap_insert(map, digest, node);
	}

	free(digest);
}

void search_all(duplicates_hashmap* output, bool ignore_dotfiles, char** directories, int directories_length) {
	recurse_directories(directories, directories_length, ignore_dotfiles, (file_handler)_search_all_callback, output);
}

