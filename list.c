#include "list.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

file_list* file_list_construct_from_file(const char* filepath) {
	file_list* node = malloc(sizeof(file_list));

	struct stat file_stat;
	if (stat(filepath, &file_stat) == -1) {
		free(node);
		fprintf(stderr, "Cannot stat %s\n", filepath);
		return NULL;
	}
	node->path = strdup(filepath);
	node->inode_number = file_stat.st_ino;
	node->size = file_stat.st_size;
	node->next = NULL;

	return node;
}

void file_list_print(file_list* head, const char* separator) {
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

void file_list_deduplicate(file_list* head) {
	const char* target_path = head->path;
	head = head->next;
	while (head != NULL) {
		unlink(head->path);
		link(head->path, target_path);
		head = head->next;
	}
}



void file_list_free(file_list* head) {
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



