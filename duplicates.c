//  CITS2002 Project 2 2021
//  Name(s):             Cormac Sharkey, Avery Warddhana
//  Student number(s):   22983427, 22984998

//  Project must be compiled with `make`

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "const.h"
#include "util.h"
#include "list.h"
#include "hashmap.h"
#include "file_recurse.h"
#include "search.h"

extern char *optarg;
extern int optind, opterr, optopt;

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
		
		bool files_found = result != NULL;
		if (!quiet) {
			file_list_print(result, "\n");
			printf("\n");
		}
		if (modify) {
			file_list_deduplicate(result);
		}
		if (quiet && files_found) {
			file_list_free(result);
			return EXIT_FAILURE;
		}
		
		file_list_free(result);
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
					file_list_deduplicate(current->files);
				}
			}
		}

		hashmap_free(&map);
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
				file_list_print(current->files, "\t");
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
				file_list_deduplicate(current->files);
			}
		}
	}
	
	hashmap_free(&map);
	return EXIT_SUCCESS;
}
