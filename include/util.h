#pragma once

#include <stddef.h>

// Parse a hex string into raw bytes
unsigned char* parse_hexstring(const char* hexstring, size_t expected_length);
