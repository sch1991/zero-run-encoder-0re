#ifndef KEYMAKER_H
#define KEYMAKER_H

#include "common.h"

#ifdef _WIN32
	#define fseeko _fseeki64
	#define ftello _ftelli64
#endif

// initialize struct Key.
void init_struct_key(Key* stp_key, FILE* fp_rb_src_file, byte b_src_file_name_len, const char* cp_src_file_name, FILE* fp1d_wrb_key_file[2]);
// write map change record & chunk's maps. (map : Most frequent 2-bit codes for each 2-bit pattern in chunk.)
static void wr_key_files(FILE* fp_rb_src_file, FILE* fp_wrb_map_chg_rec_file, FILE* fp_wrb_chunk_maps_file);
// update Key->b_key_head. (b_head_bits : 0b**000000.)
void add_head_bits(byte* bp_key_head, byte b_head_bits);
// write struct key data into 0re file.
void wr_key_to_0re(Key* stp_key, FILE* fp_wb_0re_file);
// read 0re file to set struct key & some config. (if fp1d_wrb_key_file==NULL, skip key files initalization.)
void rd_key_from_0re(Key* stp_key, FILE* fp_rb_0re_file, FILE* fp1d_wrb_key_file[2]);

#endif
