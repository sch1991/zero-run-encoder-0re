#include "converter.h"

static int load_mapping_table_set(byte* bp3d_mapping_table_set[MAPPING_TABLE_SET_SIZE][MAPPING_TABLE_1D_SIZE][MAPPING_TABLE_2D_SIZE]) {
	
	word w_idx;
	word set_idx, idx_1d, idx_2d;
	
	FILE* fp_mapping_table_set;
	
	// alloc memory for size of mapping table set and link it to each pointer in bp3d_mapping_table_set.
	bp3d_mapping_table_set[0][0][0] = malloc(MAPPING_TABLE_SET_SIZE * MAPPING_TABLE_1D_SIZE * MAPPING_TABLE_2D_SIZE * sizeof(byte));
	if(bp3d_mapping_table_set[0][0][0] == NULL) {
		puts("> Failed to allocate memory for mapping_table_set");
		return -1;
	}	st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = bp3d_mapping_table_set[0][0][0];
	for(w_idx=0, set_idx=0 ; set_idx<MAPPING_TABLE_SET_SIZE ; set_idx++) {
		for(idx_1d=0 ; idx_1d<MAPPING_TABLE_1D_SIZE ; idx_1d++) {
			for(idx_2d=0 ; idx_2d<MAPPING_TABLE_2D_SIZE ; idx_2d++) {
				bp3d_mapping_table_set[set_idx][idx_1d][idx_2d] = &bp3d_mapping_table_set[0][0][0][w_idx++];
			}
		}
	}
	
	// bp_packed_mapping_table_set : pack mapping values for all 2-bit patterns following a specific 2-bit code into 1byte.
	byte* bp_packed_mapping_table_set = malloc(MAPPING_TABLE_SET_SIZE * MAPPING_TABLE_1D_SIZE * sizeof(byte));
	if(bp_packed_mapping_table_set == NULL) {
		puts("> Failed to allocate memory for packed_mapping_table_set");
		return -1;
	}	st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = bp_packed_mapping_table_set;
	
	if(access(MAPPING_TABLE_SET_PATH, F_OK) == 0) {
		if((fp_mapping_table_set = fopen(MAPPING_TABLE_SET_PATH, "rb")) == NULL) {
			perror("> Failed to open mapping table set file");
			return -1;
		}	st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_mapping_table_set;
		
		// get bp_packed_mapping_table_set to set bp3d_mapping_table_set.
		fread(bp_packed_mapping_table_set, sizeof(byte), MAPPING_TABLE_SET_SIZE*MAPPING_TABLE_1D_SIZE, fp_mapping_table_set);
		
	} else {
		puts("> Mapping table set file not found.");
		
		byte b_idx;
		
		w_idx = b_idx = 0;
		// calc mapping values packed into a byte from 2-bit codes. (b_idx : all 8-bit chunk map patterns.)
		do {
			bp_packed_mapping_table_set[w_idx++] = BASE_MAPPING_CODES ^ 0b01010101*(b_idx>>6);
			bp_packed_mapping_table_set[w_idx++] = BASE_MAPPING_CODES ^ 0b01010101*(b_idx>>4 & 0b11);
			bp_packed_mapping_table_set[w_idx++] = BASE_MAPPING_CODES ^ 0b01010101*(b_idx>>2 & 0b11);
			bp_packed_mapping_table_set[w_idx++] = BASE_MAPPING_CODES ^ 0b01010101*(b_idx & 0b11);
			
		} while(b_idx++ < MAPPING_TABLE_SET_SIZE-1);
		
		if((fp_mapping_table_set = fopen(MAPPING_TABLE_SET_PATH, "wb")) == NULL) {
			perror("> Failed to create mapping table set file");
			return -1;
		}	st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_mapping_table_set;
			st_RESOURCES.cp1d_path_stack[st_RESOURCES.w_path_stack_top++] = MAPPING_TABLE_SET_PATH;
		
		// write mapping table set file.
		fwrite(bp_packed_mapping_table_set, sizeof(byte), MAPPING_TABLE_SET_SIZE*MAPPING_TABLE_1D_SIZE, fp_mapping_table_set);
		
		st_RESOURCES.w_path_stack_top--; // pop mapping table set path. (not to be deleted.)
		
		puts("> Mapping table set file created.");
	}
	
	fclose(st_RESOURCES.fp1d_fp_stack[--st_RESOURCES.w_fp_stack_top]); // fclose(fp_mapping_table_set);
	
	// unpack bp_packed_mapping_table_set's byte into 2-bit codes and set bp3d_mapping_table_set. (set_idx : all 8-bit chunk map patterns.)
	for(w_idx=0, set_idx=0 ; set_idx<MAPPING_TABLE_SET_SIZE ; set_idx++) {
		for(idx_1d=0 ; idx_1d<MAPPING_TABLE_1D_SIZE ; idx_1d++, w_idx++) {
			*bp3d_mapping_table_set[set_idx][idx_1d][0b00] = bp_packed_mapping_table_set[w_idx] >> 6;
			*bp3d_mapping_table_set[set_idx][idx_1d][0b01] = (bp_packed_mapping_table_set[w_idx] >> 4) & 0b11;
			*bp3d_mapping_table_set[set_idx][idx_1d][0b10] = (bp_packed_mapping_table_set[w_idx] >> 2) & 0b11;
			*bp3d_mapping_table_set[set_idx][idx_1d][0b11] = bp_packed_mapping_table_set[w_idx] & 0b11;
		}
	}
	
	free(st_RESOURCES.vp1d_heap_stack[--st_RESOURCES.w_heap_stack_top]); // free(bp_packed_mapping_table_set);
	
	return 0;
}



static inline void calc_mapping_table(byte* bp2d_mapping_table[MAPPING_TABLE_1D_SIZE][MAPPING_TABLE_2D_SIZE], const byte* bp_chunk_map) {
	
	word idx_1d;
	byte conv_mapping_codes; // (conv_mapping_codes == packed_mapping_table's codes)
	
	// calc mapping values from chunk's map and set bp2d_mapping_table. ([prev_code(index_1d)][cur_code(index_2d)] = 2-bit conversion mapping code.)
	for(idx_1d=0 ; idx_1d<MAPPING_TABLE_1D_SIZE ; idx_1d++) {
		
		// calc conversion mapping codes.
		conv_mapping_codes = BASE_MAPPING_CODES ^ 0b01010101*((*bp_chunk_map >> (0b11-idx_1d << 1)) & 0b11);
		
		*bp2d_mapping_table[idx_1d][0b00] = conv_mapping_codes >> 6;
		*bp2d_mapping_table[idx_1d][0b01] = (conv_mapping_codes >> 4) & 0b11;
		*bp2d_mapping_table[idx_1d][0b10] = (conv_mapping_codes >> 2) & 0b11;
		*bp2d_mapping_table[idx_1d][0b11] = conv_mapping_codes & 0b11;
	}
	
	return; 
}



static int load_conv_table_set(const bool* is_enc_mode, byte* bp2d_conv_table_set[CONV_TABLE_SET_SIZE][CONV_TABLE_1D_SIZE], byte* bp3d_mapping_table_set[MAPPING_TABLE_SET_SIZE][MAPPING_TABLE_1D_SIZE][MAPPING_TABLE_2D_SIZE]) {

	word w_idx;
	word set_idx, idx_1d;
	
	FILE* fp_conv_table_set;
	
	// alloc memory for size of conversion table set and link it to each pointer in bp2d_conv_table_set.
	bp2d_conv_table_set[0][0] = malloc(CONV_TABLE_SET_SIZE * CONV_TABLE_1D_SIZE * sizeof(byte));
	if(bp2d_conv_table_set[0][0] == NULL) {
		puts("> Failed to allocate memory for conv_table_set");
		return -1;
	}	st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = bp2d_conv_table_set[0][0];
	for(w_idx=0, set_idx=0 ; set_idx<CONV_TABLE_SET_SIZE ; set_idx++) {
		for(idx_1d=0 ; idx_1d<CONV_TABLE_1D_SIZE ; idx_1d++) {
			bp2d_conv_table_set[set_idx][idx_1d] = &bp2d_conv_table_set[0][0][w_idx++];
		}
	}
	
	char* cp_conv_table_set_path = *is_enc_mode ? ENC_CONV_TABLE_SET_PATH : DEC_CONV_TABLE_SET_PATH;
	
	if(access(cp_conv_table_set_path, F_OK) == 0) {
		if((fp_conv_table_set = fopen(cp_conv_table_set_path, "rb")) == NULL) {
			perror("> Failed to open conversion table set file");
			return -1;
		}	st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_conv_table_set;
		
		// get conv_table_set to set bp2d_conv_table_set.
		fread(bp2d_conv_table_set[0][0], sizeof(byte), CONV_TABLE_SET_SIZE*CONV_TABLE_1D_SIZE, fp_conv_table_set);
		
	} else {
		if(*is_enc_mode) puts("> Encoding conversion table set file not found.");
		else puts("> Decoding conversion table set file not found.");
		
		byte b_idx = 0b0;
		
		do {
			// calc converted bytes table using mapping table. (b_idx : all 8-bit chunk map patterns.)
			if(!st_CONFIG.use_mapping_table_set) {
				calc_mapping_table(bp3d_mapping_table_set[0], &b_idx);
				calc_conv_table(is_enc_mode, bp2d_conv_table_set[b_idx], bp3d_mapping_table_set[0]);
				
			} else calc_conv_table(is_enc_mode, bp2d_conv_table_set[b_idx], bp3d_mapping_table_set[b_idx]);
			
		} while(b_idx++ < CONV_TABLE_SET_SIZE-1);
		
		if((fp_conv_table_set = fopen(cp_conv_table_set_path, "wb")) == NULL) {
			perror("> Failed to create conversion table set file");
			return -1;
		}	st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_conv_table_set;
			st_RESOURCES.cp1d_path_stack[st_RESOURCES.w_path_stack_top++] = cp_conv_table_set_path;
		
		// write conversion table set file.
		fwrite(bp2d_conv_table_set[0][0], sizeof(byte), CONV_TABLE_SET_SIZE*CONV_TABLE_1D_SIZE, fp_conv_table_set);
		
		st_RESOURCES.w_path_stack_top--; // pop conversion table set path. (not to be deleted.)
		
		if(*is_enc_mode) puts("> Encoding conversion table set file created.");
		else puts("> Decoding conversion table set file created.");
	}
	
	fclose(st_RESOURCES.fp1d_fp_stack[--st_RESOURCES.w_fp_stack_top]); // fclose(fp_conv_table_set);
	
	return 0;
}



static inline void calc_conv_table(const bool* is_enc_mode, byte* bp1d_conv_table[CONV_TABLE_1D_SIZE], byte* bp2d_mapping_table[MAPPING_TABLE_1D_SIZE][MAPPING_TABLE_2D_SIZE]) {
	
	byte b_idx = 0b0;
	byte b1d_split_2bits[4]; // (1byte -> 2-bit * 4)
	
	do {
		// split input byte into 2-bit codes. (b_idx : all 8-bit input byte patterns.)
		b1d_split_2bits[0] = b_idx >> 6;
		b1d_split_2bits[1] = (b_idx >> 4) & 0b11;
		b1d_split_2bits[2] = (b_idx >> 2) & 0b11;
		b1d_split_2bits[3] = b_idx & 0b11;
		
		// convert input bytes into mapped bytes using mapping table.
		if(*is_enc_mode) {
			*bp1d_conv_table[b_idx] = *bp2d_mapping_table[b1d_split_2bits[0]][b1d_split_2bits[1]] << 4;
			*bp1d_conv_table[b_idx] |= *bp2d_mapping_table[b1d_split_2bits[1]][b1d_split_2bits[2]] << 2;
			*bp1d_conv_table[b_idx] |= *bp2d_mapping_table[b1d_split_2bits[2]][b1d_split_2bits[3]];
			
		} else {
			// b1d_split_2bits[0] = *bp2d_mapping_table[prev_mapping_2bits][b1d_split_2bits[0]]; (reverted 2bits)
			*bp1d_conv_table[b_idx] = b1d_split_2bits[0] << 6;
			b1d_split_2bits[1] = *bp2d_mapping_table[b1d_split_2bits[0]][b1d_split_2bits[1]]; // (reverted 2bits)
			*bp1d_conv_table[b_idx] |= b1d_split_2bits[1] << 4;
			b1d_split_2bits[2] = *bp2d_mapping_table[b1d_split_2bits[1]][b1d_split_2bits[2]]; // (reverted 2bits)
			*bp1d_conv_table[b_idx] |= b1d_split_2bits[2] << 2;
			b1d_split_2bits[3] = *bp2d_mapping_table[b1d_split_2bits[2]][b1d_split_2bits[3]]; // (reverted 2bits)
			*bp1d_conv_table[b_idx] |= b1d_split_2bits[3];
		}

		
	} while(b_idx++ < CONV_TABLE_1D_SIZE-1);
	
	return;
}



void wr_converted_file(const bool is_enc_mode, FILE* fp_rb_input_file, FILE* fp_wrb_output_file, FILE* fp_wrb_map_chg_rec_file, FILE* fp_wrb_chunk_maps_file) {
	
	word w_idx;
	word idx_1d, idx_2d;
	word w_read_len;
	byte rd_buffer[st_CONFIG.w_buffer_size];
	byte wr_buffer[st_CONFIG.w_buffer_size];
	
	word w_rd_rec_bit_cnt, w_rd_map_cnt, w_rd_chunk_cnt; // (w_rd_rec_bit_cnt >> 3) : rd_rec_buf_idx.
	word w_rd_rec_bit_len, w_rd_map_len;
	byte* rd_rec_buffer = malloc((st_CONFIG.w_buffer_size>>3) * sizeof(byte));
	if(rd_rec_buffer == NULL) cleanup_resources(-1, "> Failed to allocate memory for rd_rec_buffer"); // exit(-1);
		st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = rd_rec_buffer;
	byte* rd_map_buffer = malloc(st_CONFIG.w_buffer_size * sizeof(byte));
	if(rd_map_buffer == NULL) cleanup_resources(-1, "> Failed to allocate memory for rd_map_buffer"); // exit(-1);
		st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = rd_map_buffer;
	
	// if use_table_set is true, declare array by table set size. else, declare by single table size.
	byte* bp3d_mapping_table_set[st_CONFIG.use_mapping_table_set ? MAPPING_TABLE_SET_SIZE : 1][MAPPING_TABLE_1D_SIZE][MAPPING_TABLE_2D_SIZE];
	byte* bp2d_conv_table_set[st_CONFIG.use_conv_table_set ? CONV_TABLE_SET_SIZE : 1][CONV_TABLE_1D_SIZE];
	
	if(st_CONFIG.use_mapping_table_set) {
		// if use_table_set is true, init bp3d_mapping_table_set with table set values.
		if(load_mapping_table_set(bp3d_mapping_table_set) < 0) cleanup_resources(-1, "> Failed to load mapping table set\n"); // exit(-1);
	} else {
		// if use_table_set is false, alloc memory for single table size and link it to each array's pointer.
		bp3d_mapping_table_set[0][0][0] = malloc(MAPPING_TABLE_1D_SIZE * MAPPING_TABLE_2D_SIZE * sizeof(byte));
		if(bp3d_mapping_table_set[0][0][0] == NULL) cleanup_resources(-1, "> Failed to allocate memory for mapping_table_set"); // exit(-1);
			st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = bp3d_mapping_table_set[0][0][0];
		for(w_idx=0, idx_1d=0 ; idx_1d<MAPPING_TABLE_1D_SIZE ; idx_1d++) {
			for(idx_2d=0 ; idx_2d<MAPPING_TABLE_2D_SIZE ; idx_2d++) {
				bp3d_mapping_table_set[0][idx_1d][idx_2d] = &bp3d_mapping_table_set[0][0][0][w_idx++];
			}
		}
	}
	
	if(st_CONFIG.use_conv_table_set) {
		// if use_table_set is true, init bp3d_mapping_table_set with table set values.
		if(load_conv_table_set(&is_enc_mode, bp2d_conv_table_set, bp3d_mapping_table_set) < 0) cleanup_resources(-1, "> Failed to load conversion table set\n"); // exit(-1);
	} else {
		// if use_table_set is false, alloc memory for single table size and link it to each array's pointer.
		bp2d_conv_table_set[0][0] = malloc(CONV_TABLE_1D_SIZE * sizeof(byte));
		if(bp2d_conv_table_set[0][0] == NULL) cleanup_resources(-1, "> Failed to allocate memory for conv_table_set"); // exit(-1);
			st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = bp2d_conv_table_set[0][0];
		for(idx_1d=0 ; idx_1d<CONV_TABLE_1D_SIZE ; idx_1d++) {
			bp2d_conv_table_set[0][idx_1d] = &bp2d_conv_table_set[0][0][idx_1d];
		}
	}
	
	puts("> File conversion in progress...");
	
	// reset read file pointers & init progress bar.
	fseek(fp_rb_input_file, 0, SEEK_SET);
	fseek(fp_wrb_map_chg_rec_file, 0, SEEK_SET);
	fseek(fp_wrb_chunk_maps_file, 0, SEEK_SET);
	draw_progress_bar(fp_rb_input_file);
	
	// reset write file pointer & truncate output file to 0 byte.
	fseek(fp_wrb_output_file, 0, SEEK_SET);
	ftruncate(fileno(fp_wrb_output_file), 0);
	
	// declare array pointers for handling tbales in table set.
	byte* (*bp1dp_mapping_table)[MAPPING_TABLE_2D_SIZE] = &bp3d_mapping_table_set[0][0];
	byte* (*bp0dp_conv_table) = &bp2d_conv_table_set[0][0];
	
	bool is_head = true;
	byte b_prev_2bit;
	
	w_rd_rec_bit_cnt = w_rd_map_cnt = w_rd_chunk_cnt = 0;
	w_rd_rec_bit_len = w_rd_map_len = 0;
	
	// convert input file.
	while((w_read_len = fread(rd_buffer, sizeof(byte), sizeof(rd_buffer), fp_rb_input_file)) > 0) {
		
		for(w_idx=0 ; w_idx<w_read_len ; w_idx++, w_rd_chunk_cnt--) {
			
			// check rd_rec_buffer's used record(=bit units) count.
			if(w_rd_rec_bit_cnt == w_rd_rec_bit_len) {
				if((w_rd_rec_bit_len = fread(rd_rec_buffer, sizeof(byte), st_CONFIG.w_buffer_size>>3, fp_wrb_map_chg_rec_file)) > 0) {
					w_rd_rec_bit_len <<= 3;
					w_rd_rec_bit_cnt = 0;
					
				} else w_rd_rec_bit_cnt = ~w_rd_rec_bit_len; // if fp_wrb_map_chg_rec_file == EOF, stop checking buffer.
			}
			// check rd_map_buffer's used map(=byte units) count.
			if(w_rd_map_cnt == w_rd_map_len) {
				if((w_rd_map_len = fread(rd_map_buffer, sizeof(byte), st_CONFIG.w_buffer_size, fp_wrb_chunk_maps_file)) > 0) {
					w_rd_map_cnt = 0;
					
				} else w_rd_map_cnt = ~w_rd_map_len; // if fp_wrb_chunk_maps_file == EOF, stop checking buffer.
			}
			// check remaing lifetime of current map. (map's lifetime : chunk size.)
			if(w_rd_chunk_cnt == 0) {
				if(((rd_rec_buffer[w_rd_rec_bit_cnt>>3] >> (~w_rd_rec_bit_cnt&0b111)) & 0b1) == 0) {
					// (if map_chg_rec == 1) set mapping_table to next map's mapping table.
					if(st_CONFIG.use_mapping_table_set) {
						bp1dp_mapping_table = &bp3d_mapping_table_set[rd_map_buffer[w_rd_map_cnt]][0];
					} else calc_mapping_table(bp1dp_mapping_table, &rd_map_buffer[w_rd_map_cnt]);
					// (if map_chg_rec == 1) set conv_table to next map's conversion table.
					if(st_CONFIG.use_conv_table_set) {
						bp0dp_conv_table = &bp2d_conv_table_set[rd_map_buffer[w_rd_map_cnt]][0];
					} else calc_conv_table(&is_enc_mode, bp0dp_conv_table, bp1dp_mapping_table);
					
					w_rd_map_cnt++;
				}
				w_rd_rec_bit_cnt++;
				w_rd_chunk_cnt = st_CONFIG.w_chunk_size;
			}
			
			if(is_enc_mode) {
				// get converted byte(0b00******) from enc_conv_table[input_byte].
				wr_buffer[w_idx] = *bp0dp_conv_table[rd_buffer[w_idx]];
				
				if(is_head) {
					wr_buffer[w_idx] |= rd_buffer[w_idx] & 0b11000000;
					is_head = !is_head;
				} else wr_buffer[w_idx] |= *bp1dp_mapping_table[b_prev_2bit][rd_buffer[w_idx]>>6] << 6; // fill converted byte's head 2bits.
				
				b_prev_2bit = rd_buffer[w_idx] & 0b11; // store input byte's tail 2bits.
				
			} else {
				// get converted byte.
				if(is_head) {
					wr_buffer[w_idx] = *bp0dp_conv_table[rd_buffer[w_idx]]; // from dec_conv_table[input_bits]
					is_head = !is_head;
				} else wr_buffer[w_idx] = *bp0dp_conv_table[*bp1dp_mapping_table[b_prev_2bit][rd_buffer[w_idx]>>6]<<6 | (rd_buffer[w_idx]&0b00111111)]; // from dec_conv_table[reverted 2bits + input 6bits]
				
				b_prev_2bit = wr_buffer[w_idx] & 0b11; // store converted byte's tail 2bits.
			}
		}
		
		// write converted data & update progress bar.
		fwrite(wr_buffer, sizeof(byte), w_read_len, fp_wrb_output_file);
		draw_progress_bar(fp_rb_input_file);
	}
	
	fflush(fp_wrb_output_file);
	
	// free all allocated heap memory used for conversion.
	free(st_RESOURCES.vp1d_heap_stack[--st_RESOURCES.w_heap_stack_top]); // free(bp2d_conv_table_set[0][0]);
	free(st_RESOURCES.vp1d_heap_stack[--st_RESOURCES.w_heap_stack_top]); // free(bp3d_mapping_table_set[0][0][0]);
	free(st_RESOURCES.vp1d_heap_stack[--st_RESOURCES.w_heap_stack_top]); // free(rd_map_buffer);
	free(st_RESOURCES.vp1d_heap_stack[--st_RESOURCES.w_heap_stack_top]); // free(rd_rec_buffer);
	
	return;
}



byte extr_head_bits(FILE* fp_wrb_temp_file) {
	
	byte b_head_byte;
	byte b_head_bits; // 2bits.
	
	fseek(fp_wrb_temp_file, 0, SEEK_SET);
	fread(&b_head_byte, sizeof(byte), 1, fp_wrb_temp_file);
	
	// split b_head_byte to head(2bits) and rest(6bits).
	b_head_bits = b_head_byte & 0b11000000;
	b_head_byte -= b_head_bits;
	
	fseek(fp_wrb_temp_file, 0, SEEK_SET);
	fwrite(&b_head_byte, sizeof(byte), 1, fp_wrb_temp_file);
	
	fflush(fp_wrb_temp_file);
	
	return b_head_bits;
}



void set_head_bits(byte b_head_bits, FILE* fp_wrb_temp_file) {
	
	byte b_head_byte;
	
	fseek(fp_wrb_temp_file, 0, SEEK_SET);
	fread(&b_head_byte, sizeof(byte), 1, fp_wrb_temp_file);
	
	if(b_head_byte>>6 == 0b00) {
		// set head bits in decoded file's head byte.
		b_head_byte |= b_head_bits;
		fseek(fp_wrb_temp_file, 0, SEEK_SET);
		fwrite(&b_head_byte, sizeof(byte), 1, fp_wrb_temp_file);
		
	} else cleanup_resources(-1, "> Failed to decode 0re file: Invalid head bits in decoded file'\n"); // exit(-1);
	
	return;
}
