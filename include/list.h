#pragma once

#include <stddef.h>
#include <sys/types.h>

typedef struct file_list_node_t {
	struct file_list_node_t* next;
	char* path;
	// A hard link is defined by pointing to the same inode number.
	// We'll need this to count sizes when hard-links are considered.
	ino_t inode_number;
	size_t size;
} file_list;

// From a filepath, construct a list node for file_list
file_list* file_list_construct_from_file(const char* filepath);

// Print each element of a given file_list, seperated by a specified string
void file_list_print(file_list* head, const char* separator);

// Simplify duplicate files by deleting other files and hard-linking back to the original
void file_list_deduplicate(file_list* head);

// Free a file list
void file_list_free(file_list* head);
