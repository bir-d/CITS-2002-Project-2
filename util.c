#include "util.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

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
