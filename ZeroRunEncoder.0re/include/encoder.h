#ifndef ENCODER_H
#define ENCODER_H

#include "common.h"

#if __SIZEOF_POINTER__ == 8
	#define cnt_leading_zero __builtin_clzll
	#define byte_swap __builtin_bswap64
#else
	#define cnt_leading_zero __builtin_clz
	#define byte_swap __builtin_bswap32
#endif

#define IS_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

#define ENCODING_TABLE_SIZE 256
#define LOG2(x) (sizeof(word)<<3) - cnt_leading_zero(x)
#define LOG2LL(x) (sizeof(qword)<<3) - __builtin_clzll(x)

typedef struct Encoding_Table {
	byte b_head_run_len;
	byte b_tail_run_len;
	byte b_encoded_bits_len; // excluding tail run.
	word w_encoded_bits; // filled from MSB & excluding tail run.
} Encoding_Table;

typedef union Enc_Buffer {
	byte* b_buffer;
	word* w_buffer;
} Enc_Buffer;

// load encoding table or create 'encoding_table.bin' and initialize st1d_ENCODING_TABLE.
int load_encoding_table();
// encode input file bits and write to output file. (if use_enc_cnt, encode for input_file; otherwise, encode for map_chg_rec.)
void wr_encoded_file(const bool use_enc_cnt, FILE* fp_wrb_input_file, FILE* fp_wrb_output_file);

#endif
