#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "const.h"
#include "list.h"
#include "hashmap.h"
#include "file_recurse.h"

extern int SHA2(const char *filename, unsigned char *output_digest);

typedef struct search_by_hash_or_filename_state_t {
	const unsigned char* target_digest;
	const char* target_filepath; // Used only in the context of search_by_filename
	file_list* head;
	file_list* tail;
} search_state;

void _search_by_hash_or_filename_callback(const char* filepath, search_state* parameters);
file_list* search_by_hash(const unsigned char* digest, bool ignore_dotfiles, char** directory_list, int directory_list_length);
file_list* search_by_filename(const char* filename, bool ignore_dotfiles, char** directory_list, int directory_list_length);

void _search_all_callback(const char* filename, duplicates_hashmap* map);
void search_all(duplicates_hashmap* output, bool ignore_dotfiles, char** directory_list, int directory_list_length);
