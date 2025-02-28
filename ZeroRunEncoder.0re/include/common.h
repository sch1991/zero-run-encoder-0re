#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h> // for va_list.
#include <string.h>
#include <stdlib.h>
#include <unistd.h> // for access(), ftruncate().
#include <pthread.h>
#include <signal.h>

#ifdef _WIN32
	#define mkdir(path, mode) _mkdir(path)
	#define PS "\\" // window's path separator.
#else
	#define PS "/" // other path separator.
#endif

#define ZERORE_SIGNATURE (uint32_t)0x30726521 // 0re file's header. (big-endian ASCII "0re!")

#define CONFIG_PATH				 "." PS "config.ini"
#define TEMP_DIR_PATH			 "." PS "temp"
#define BIN_DIR_PATH			 "." PS "bin"
#define KEY_PATH(idx)			 TEMP_DIR_PATH PS "key_file.key" #idx // .key0 : map change record, .key1 : chunk's maps.
#define TEMP_PATH(idx)			 TEMP_DIR_PATH PS "temp_file.tmp" #idx
#define MAPPING_TABLE_SET_PATH	 BIN_DIR_PATH PS "mapping_table_set.bin"
#define ENC_CONV_TABLE_SET_PATH	 BIN_DIR_PATH PS "enc_conv_table_set.bin"
#define DEC_CONV_TABLE_SET_PATH	 BIN_DIR_PATH PS "dec_conv_table_set.bin"
#define ENC_TABLE_PATH			 BIN_DIR_PATH PS "encoding_table.bin"

#define ENCODING_MODE		 "encoding"
#define DECODING_MODE		 "decoding"
#define SHOW_DETAILS_MODE	 "details"

#define KiB_OFFSET 10
#define MiB_OFFSET 20

#define FP_STACK_SIZE	 10
#define PATH_STACK_SIZE	 8
#define HEAP_STACK_SIZE	 8
#define THRD_STACK_SIZE	 1

typedef uint8_t byte; // 8bits.
typedef uint16_t hword; // 16bits. (2bytes)
typedef size_t word; // 32bits or 64bits. (depending on system architecture.)
typedef ssize_t s_word; // signed word.
typedef uint64_t qword; // 64bits.
typedef int64_t s_qword; // signed qword.

typedef struct Config {
	word w_encoding_cnt;
	word w_buffer_size;
	word w_chunk_size;
	word w_chunk_offset;
	bool use_mapping_table_set;
	bool use_conv_table_set;
	bool use_file_size_limit;
} Config;

extern Config st_CONFIG;

typedef struct Resources {
	FILE* fp1d_fp_stack[FP_STACK_SIZE]; // opened file pointer stack.
	word w_fp_stack_top;
	char* cp1d_path_stack[PATH_STACK_SIZE]; // created file path stack.
	word w_path_stack_top;
	void* vp1d_heap_stack[HEAP_STACK_SIZE]; // heap-allocated variable stack.
	word w_heap_stack_top;
	pthread_t thrd1d_thrd_stack[THRD_STACK_SIZE]; // created thread's id stack.
	word w_thrd_stack_top;
} Resources; // for resource management.

extern Resources st_RESOURCES;

typedef struct Key {
	byte b_key_head; // 0b(2bits : encoding count. 0~3)(2bits * 0~3 : head_bits * 0~3, filled from MSB)
	byte b_src_file_name_len; // max length : 255. (excluding '\0')
	const char* cp_src_file_name;
	qword qw_src_file_size; // in bytes.
	byte b_chunk_size; // 0b(5bits : 0~31 +1 byte chunk size.)(3bits : 0~7 offset to scale chunk size.)
	FILE* fp_map_chg_rec_file; // map change record. (change : 0b0, not change : 0b1, filled from MSB)
	FILE* fp_chunk_maps_file; // chunk's maps. (map : 0b(00->)**(01->)**(10->)**(11->)**)
} Key;

#endif
