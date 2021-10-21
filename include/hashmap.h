#pragma once

#include "const.h"
#include "list.h"

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

// Initialise a hashmap with a specified number of buckets
void hashmap_init(duplicates_hashmap* hashmap, int length);

// Derive an index into a given hashmap from the raw hash digest.
unsigned int _hashmap_index_from_hash(duplicates_hashmap* map, unsigned char* hash);

// Insert a file_list node "value" into a hashmap using the hash as the "key"
void hashmap_insert(duplicates_hashmap* map, unsigned char* hash, file_list* node);

// Free a hashmap
void hashmap_free(duplicates_hashmap* map);
