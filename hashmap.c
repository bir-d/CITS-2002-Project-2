#include "hashmap.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void hashmap_init(duplicates_hashmap* hashmap, int length) {
	hashmap->buckets_length = length;
	hashmap->buckets = calloc(length, sizeof(hashmap_entry*));
}

unsigned int _hashmap_index_from_hash(duplicates_hashmap* map, unsigned char* hash) {
	unsigned int hash_int = 0;
	for (int i = 0; i < HASH_DIGEST_SZ; i++) {
		hash_int <<= 8;
		hash_int |= hash[i];
	}

	return hash_int % map->buckets_length;
}

void hashmap_insert(duplicates_hashmap* map, unsigned char* hash, file_list* node) {
	unsigned int index = _hashmap_index_from_hash(map, hash);

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

void hashmap_free(duplicates_hashmap* map) {
	for (int i = 0; i < map->buckets_length; i++) {
		if (map->buckets[i] == NULL) {
			continue;
		}

		file_list_free(map->buckets[i]->files);
		free(map->buckets[i]);
	}

	free(map->buckets);

	map->buckets_length = 0;
	map->buckets = NULL;
}

