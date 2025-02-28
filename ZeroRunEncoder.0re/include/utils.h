#ifndef UTILS_H
#define UTILS_H

#include "common.h"

#ifdef _WIN32
	#define fseeko _fseeki64
	#define ftello _ftelli64
#endif

#if __SIZEOF_POINTER__ == 8
	#define strtozu strtoull
#else
	#define strtozu strtoul
#endif

#define CONFIG_MAX_LEN 1024

// ENCODING_COUNT.
#define DEFAULT_ENCODING_COUNT (word)1
#define MAX_ENCODING_COUNT (word)0b11
// BUFFER_SIZE.
#define BUFFER_UNIT_OFFSET	 KiB_OFFSET
#define DEFAULT_BUFFER_SIZE	 (word)0x0400 << BUFFER_UNIT_OFFSET // 1MiB.
#define MAX_BUFFER_SIZE		 (word)0x6000 << BUFFER_UNIT_OFFSET // 24MiB.
#define MIN_BUFFER_SIZE		 (word)0x0040 << BUFFER_UNIT_OFFSET // 64KiB.
// CHUNK_SIZE & CHUNK_OFFSET.
#define DEFAULT_CHUNK_SIZE	 (word)0b100000 // 32Byte.
#define MAX_CHUNK_SIZE		 (word)0b100000 // 32Byte.
#define MIN_CHUNK_SIZE		 (word)0b000001 // 1Byte.
#define DEFAULT_CHUNK_OFFSET (word)0b000
#define MAX_CHUNK_OFFSET	 (word)0b111
#define MIN_CHUNK_OFFSET	 (word)0b000
// USE_MAPPING_TABLE_SET & USE_CONV_TABLE_SET.
#define DEFAULT_USE_MAPPING_TABLE_SET true
#define DEFAULT_USE_CONV_TABLE_SET true
// USE_FILE_SIZE_LIMIT.
#define DEFAULT_USE_FILE_SIZE_LIMIT true

#define PROG_BAR_OFFSET 1 // adjust progress bar length. (0b01100011 >> PROG_BAR_OFFSET)
#define LOG2LL(x) (sizeof(qword)<<3) - __builtin_clzll(x)

// load configuration into st_CONFIG or create 'config.ini'.
int load_config();
// draw progress bar and current progress on the prompt.
void draw_progress_bar(FILE* fp_wrb_prog_file);
// clean up resources(pthread_cancel(), fclose(), remove(), free()) and exit(exit_code). if cp_error_msg != NULL, print error_msg. (error_msg : if contain '\n', vprintf(msg); else if errno is set, perror(msg); else, puts(msg).)
void cleanup_resources(int exit_code, const char* cp_error_msg, ...);

#endif
