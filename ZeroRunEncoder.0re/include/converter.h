#ifndef CONVERTER_H
#define CONVERTER_H

#include "common.h"

// MAPPING_TABLE_SET.
#define MAPPING_TABLE_SET_SIZE	 256 // number of 8-bit chunk map patterns.
#define MAPPING_TABLE_1D_SIZE	 4 // number of 2-bit input code patterns.
#define MAPPING_TABLE_2D_SIZE	 4 // number of 2-bit next input code patterns.
#define BASE_MAPPING_CODES (byte)0b00011011 // (BASE_MAPPING_CODES' 2-bit code) ^ (map's 2-bit code) == conv_mapping_code.
// CONV_TABLE_SET.
#define CONV_TABLE_SET_SIZE	 256 // number of 8-bit chunk map patterns.
#define CONV_TABLE_1D_SIZE	 256 // number of 8-bit input byte patterns.

// load mapping table set or create 'mapping_table_set.bin' and initialize bp3d_mapping_table_set.
static int load_mapping_table_set(byte* bp3d_mapping_table_set[MAPPING_TABLE_SET_SIZE][MAPPING_TABLE_1D_SIZE][MAPPING_TABLE_2D_SIZE]);
// calculate mapping table to maximize the number of 0-runs & set bp2d_mapping_table.
static inline void calc_mapping_table(byte* bp2d_mapping_table[MAPPING_TABLE_1D_SIZE][MAPPING_TABLE_2D_SIZE], const byte* bp_chunk_map);
// load conversion table set or create 'enc_conv_table_set.bin' or 'dec_conv_table_set.bin' and initalize bp2d_conv_table_set.
static int load_conv_table_set(const bool* is_enc_mode, byte* bp2d_conv_table_set[CONV_TABLE_SET_SIZE][CONV_TABLE_1D_SIZE], byte* bp3d_mapping_table_set[MAPPING_TABLE_SET_SIZE][MAPPING_TABLE_1D_SIZE][MAPPING_TABLE_2D_SIZE]);
// calculate conversion table for converting input byte to converted byte(0b________->0b00******) or reverted byte(0b**______->0b********) using bp2d_mapping_table & set bp1d_conv_table.
static inline void calc_conv_table(const bool* is_enc_mode, byte* bp1d_conv_table[CONV_TABLE_1D_SIZE], byte* bp2d_mapping_table[MAPPING_TABLE_1D_SIZE][MAPPING_TABLE_2D_SIZE]);
// convert source file and write to converted file.
void wr_converted_file(const bool is_enc_mode, FILE* fp_rb_input_file, FILE* fp_wrb_output_file, FILE* fp_wrb_map_chg_rec_file, FILE* fp_wrb_chunk_maps_file);
// extract head bits in 0b**000000 & set head_bits to 0b00______. (return b_head_bits.)
byte extr_head_bits(FILE* fp_wrb_temp_file);
// set head bits to 0b**(b_head_bits)______.
void set_head_bits(byte b_head_bits, FILE* fp_wrb_temp_file);

#endif
