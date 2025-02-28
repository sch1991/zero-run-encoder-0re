#include "encoder.h"

Encoding_Table st1d_ENCODING_TABLE[ENCODING_TABLE_SIZE];

int load_encoding_table() {
	
	s_word s_idx;
	byte b_idx;
	byte b_prev_bit;
		
	FILE* fp_encoding_table;
	
	// hw_encoding_table : 0x(12bits from MSB : encoded bits excluding the tail run.)(4bits from LSB : encoded bits length.) 
	hword hw_encoding_table[ENCODING_TABLE_SIZE] = {0x00, };
	
	if(access(ENC_TABLE_PATH, F_OK) == 0) {
		if((fp_encoding_table = fopen(ENC_TABLE_PATH, "rb")) == NULL) {
			perror("> Failed to open encoding table file");
			return -1;
		}	st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_encoding_table;
		
		// get hw_encoding_table to set st1d_ENCODING_TABLE.
		fread(hw_encoding_table, sizeof(hword), ENCODING_TABLE_SIZE, fp_encoding_table);
		
	} else {
		puts("> Encoding table file not found.");
		
		hword hw_run_cnt, hw_enc_bits_len;
		
		b_idx = 0;
		// calc hw_encoding_table.
		do {
			b_prev_bit = b_idx >> 7;
			hw_run_cnt = 1;
			for(hw_enc_bits_len=0, s_idx=6 ; s_idx>=0 ; s_idx--) {
				// check bit change. (0 or 1) 
				if(b_prev_bit != ((b_idx>>s_idx) & 0b1)) {
					if(b_prev_bit) {
						// encoding 1-run.
						hw_enc_bits_len += hw_run_cnt;
						hw_encoding_table[b_idx] |= 0b1 << (16 - hw_enc_bits_len);
						
					} else {
						// encoding 0-run, if 0-run then hw_run_cnt--.
						if(hw_run_cnt-- > 2) {
							 // note : run_cnt is decremented by 1.
							hw_enc_bits_len += (LOG2(hw_run_cnt) << 1) - 1;
							hw_encoding_table[b_idx] |= hw_run_cnt << (16 - hw_enc_bits_len);
						} else {
							 // note : run_cnt is decremented by 1.
							hw_enc_bits_len += 2;
							hw_encoding_table[b_idx] |= (0b10 | hw_run_cnt) << (16 - hw_enc_bits_len);
						}
					}
					
					b_prev_bit = !b_prev_bit;
					hw_run_cnt = 1;
					
				} else hw_run_cnt++;
			}
			
			hw_encoding_table[b_idx] |= hw_enc_bits_len;
			
		} while(b_idx++ < ENCODING_TABLE_SIZE-1);
		
		if((fp_encoding_table = fopen(ENC_TABLE_PATH, "wb")) == NULL) {
			perror("> Failed to create encoding table file");
			return -1;
		}	st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_encoding_table;
			st_RESOURCES.cp1d_path_stack[st_RESOURCES.w_path_stack_top++] = ENC_TABLE_PATH;
		
		// write encoding table file.
		fwrite(hw_encoding_table, sizeof(hword), ENCODING_TABLE_SIZE, fp_encoding_table);
		
		st_RESOURCES.w_path_stack_top--; // pop encoding table path. (not to be deleted.)
		
		puts("> Encoding table file created.");
	}
	
	fclose(st_RESOURCES.fp1d_fp_stack[--st_RESOURCES.w_fp_stack_top]); // fclose(fp_encoding_table);
	
	b_idx = 0;
	// init st1d_ENCODING_TABLE.
	do {
		// calc head run length.
		for(s_idx=6, b_prev_bit=b_idx>>7 ; s_idx>=0 && b_prev_bit==((b_idx>>s_idx) & 0b1) ; s_idx--);
		st1d_ENCODING_TABLE[b_idx].b_head_run_len = 7 - s_idx;
		// calc tail run length.
		for(s_idx=1, b_prev_bit=b_idx&0b1 ; s_idx<8 && b_prev_bit==((b_idx>>s_idx) & 0b1) ; s_idx++);
		st1d_ENCODING_TABLE[b_idx].b_tail_run_len = s_idx;
		// split hw_encoding_table into length & code.
		st1d_ENCODING_TABLE[b_idx].b_encoded_bits_len = hw_encoding_table[b_idx] & 0b1111;
		st1d_ENCODING_TABLE[b_idx].w_encoded_bits = (word)(hw_encoding_table[b_idx]>>4) << (sizeof(word)*8 -12);
		
	} while(b_idx++ < ENCODING_TABLE_SIZE-1);
	
	return 0;	
}



void wr_encoded_file(const bool use_enc_cnt, FILE* fp_wrb_input_file, FILE* fp_wrb_output_file) {

	static word w_enc_cnt = 0;
	
	word idx;
	word w_read_len;
	byte rd_buffer[st_CONFIG.w_buffer_size];
	Enc_Buffer uni_wr_buffer;
	const word w_uni_b_buffer_size = st_CONFIG.w_buffer_size & ~(sizeof(word)-1);
	// assign (byte array)wr_buffer to (union byte & word)uni_wr_buffer.
	byte wr_buffer[w_uni_b_buffer_size]; uni_wr_buffer.b_buffer = wr_buffer;
	
	
	// define 'fill wr_buffer and write encoded file' inner function. (x : 'prev tail run' or 'prev tail + head run', y : encoding table value for 'rest bits'.)
	void fill_buf_and_wr_file(const byte* x_run_bit, const qword* x_run_len, word y_enc_bits, word y_enc_bits_len) {
		
		static const qword qw_w_bit_cnt_mask = (sizeof(word) << 3) - 1; // use to get bit_cnt in current word.
		static const qword qw_w_bit_offset = LOG2LL(sizeof(word) << 2); // word size (in bit unit) offset, use to get word_idx.
		
		static qword qw_wr_buf_bit_cnt = 0; // (wr_bit_cnt >> bit_offset) : wr_w_buf_idx, (wr_bit_cnt & mask) : bit_cnt in current wr_w_buf.
		
		word w_fill_idx, w_buf_rest_len;
		qword qw_enc_x_pref_len, qw_enc_x_suff_len, qw_enc_x_suff_bits;
		
		if(*x_run_bit){
			// spilt encoded 1-run into prefix(0 bits), suffix(LSB ordering).
			if(*x_run_len > 0) {
				qw_enc_x_pref_len = *x_run_len - 1;
				qw_enc_x_suff_len = 1;
				qw_enc_x_suff_bits = 0b1;
				
			} else qw_enc_x_pref_len = qw_enc_x_suff_len = qw_enc_x_suff_bits = 0; // first write. (run_bit==1 && run_len==0)
		} else {
			// spilt encoded 0-run into prefix(0 bits), suffix(LSB ordering).
			if(*x_run_len > 2) {
				qw_enc_x_pref_len = LOG2LL(*x_run_len-1) - 1;
				qw_enc_x_suff_len = qw_enc_x_pref_len + 1;
				qw_enc_x_suff_bits = *x_run_len - 1;
			} else {
				qw_enc_x_pref_len = 0;
				qw_enc_x_suff_len = 2;
				qw_enc_x_suff_bits = 0b01 + *x_run_len;
			}
		}


		// define 'write wr_buffer to encoded file' inner-inner function.
		void wr_buf_to_file(word w_write_len) {
			
			static const word w_w_byte_offset = (sizeof(word)>>2) + 1; // word size (in byte unit) offset.
			
			word w_wr_idx;
			
			// if little-endian, convert union's words to big-endian.
			if(IS_LITTLE_ENDIAN) {
				for(w_wr_idx=0 ; w_wr_idx<<w_w_byte_offset<w_write_len ; w_wr_idx++)
					uni_wr_buffer.w_buffer[w_wr_idx] = byte_swap(uni_wr_buffer.w_buffer[w_wr_idx]);
			}
			
			do {
				// write wr_buffer data into output file.
				fwrite(uni_wr_buffer.b_buffer, sizeof(byte), w_write_len, fp_wrb_output_file);
				
				if(w_write_len == w_uni_b_buffer_size) {
					// reduce wr_bit_cnt by written amount & reset wr_buffer.
					qw_wr_buf_bit_cnt -= (qword)w_uni_b_buffer_size << 3;
					memset(uni_wr_buffer.b_buffer, 0x00, w_uni_b_buffer_size);
					
				} else qw_wr_buf_bit_cnt = 0; // reset (static)qw_wr_buf_bit_cnt if last write.
				
			} while(qw_wr_buf_bit_cnt>>3 >= w_uni_b_buffer_size);
			
			return;
		}

		
		// write encoded bits' prefix.
		if(qw_enc_x_pref_len > 0) {
			qw_wr_buf_bit_cnt += qw_enc_x_pref_len;
			
			if(qw_wr_buf_bit_cnt>>3 >= w_uni_b_buffer_size) wr_buf_to_file(w_uni_b_buffer_size);
		}
		
		// write encoded bits' suffix. (iterate until rest suffix length==0.)
		for(w_fill_idx=qw_wr_buf_bit_cnt>>qw_w_bit_offset ; qw_enc_x_suff_len>0 ; w_fill_idx=qw_wr_buf_bit_cnt>>qw_w_bit_offset) {
			// rest length in current buffer's word. (cnt_mask+1 : word size in bit unit.)
			w_buf_rest_len = (qw_w_bit_cnt_mask+1) - (qw_wr_buf_bit_cnt&qw_w_bit_cnt_mask);
			
			if(w_buf_rest_len < qw_enc_x_suff_len) {
				// write part of suffix.
				uni_wr_buffer.w_buffer[w_fill_idx] |= (word)(qw_enc_x_suff_bits >> (qw_enc_x_suff_len -= w_buf_rest_len));
				qw_wr_buf_bit_cnt += w_buf_rest_len;
			} else {
				// write all rest suffix bits.
				uni_wr_buffer.w_buffer[w_fill_idx] |= (word)(qw_enc_x_suff_bits << (w_buf_rest_len - qw_enc_x_suff_len));
				qw_wr_buf_bit_cnt += qw_enc_x_suff_len;
				qw_enc_x_suff_len = 0;
			}
			
			if(qw_wr_buf_bit_cnt>>3 >= w_uni_b_buffer_size) wr_buf_to_file(w_uni_b_buffer_size);	
		}
		
		// write encoding_table's encoded bits(MSB ordering).
		if(y_enc_bits_len > 0) {
			w_buf_rest_len = (qw_w_bit_cnt_mask+1) - (qw_wr_buf_bit_cnt&qw_w_bit_cnt_mask);
			
			// write part of encoded bits.
			if(w_buf_rest_len < y_enc_bits_len) {
				uni_wr_buffer.w_buffer[qw_wr_buf_bit_cnt>>qw_w_bit_offset] |= y_enc_bits >> (qw_wr_buf_bit_cnt&qw_w_bit_cnt_mask);
				qw_wr_buf_bit_cnt += w_buf_rest_len;
				
				if(qw_wr_buf_bit_cnt>>3 >= w_uni_b_buffer_size) wr_buf_to_file(w_uni_b_buffer_size);
				
				// remove bits as written.
				y_enc_bits <<= w_buf_rest_len;
				y_enc_bits_len -= w_buf_rest_len;
			}
			// write all rest encoded bits.
			uni_wr_buffer.w_buffer[qw_wr_buf_bit_cnt>>qw_w_bit_offset] |= y_enc_bits >> (qw_wr_buf_bit_cnt&qw_w_bit_cnt_mask);
			qw_wr_buf_bit_cnt += y_enc_bits_len;
			
			if(qw_wr_buf_bit_cnt>>3 >= w_uni_b_buffer_size) wr_buf_to_file(w_uni_b_buffer_size);
			
		} else if(y_enc_bits) wr_buf_to_file(qw_wr_buf_bit_cnt+0b111 >> 3); // last write. (enc_bits_len==0 && enc_bits!=0b0)
		
		return;
	}
	
	
	if(use_enc_cnt) {
		if(w_enc_cnt++ == 0) {
			puts("> Data encoding in progress...");
			
		} else if(w_enc_cnt > st_CONFIG.w_encoding_cnt) cleanup_resources(-1, "> Exceeded encoding count\n"); // exit(-1);
	
		// reset read file pointer & init progress bar.
		fseek(fp_wrb_input_file, 0, SEEK_SET);
		draw_progress_bar(fp_wrb_input_file);
	
		// reset write file pointer & truncate output file to 0 byte.
		fseek(fp_wrb_output_file, 0, SEEK_SET);
		ftruncate(fileno(fp_wrb_output_file), 0);
		
	} else {
		// only reset read file pointer & init progress bar.
		fseek(fp_wrb_input_file, 0, SEEK_SET);
		draw_progress_bar(fp_wrb_input_file);
	}
	
	byte b_prev_tail_bit = 0b1;
	qword qw_prev_tail_len = 0;
	
	word w_head_bits_len, w_enc_bits, w_enc_bits_len;
	
	memset(uni_wr_buffer.b_buffer, 0x00, w_uni_b_buffer_size);
	
	// encode input file & write to output file in byte unit.
	while((w_read_len = fread(rd_buffer, sizeof(byte), sizeof(rd_buffer), fp_wrb_input_file)) > 0) {
		
		for(idx=0 ; idx<w_read_len ; idx++) {
			if(b_prev_tail_bit == rd_buffer[idx]>>7) {
				
				if(st1d_ENCODING_TABLE[rd_buffer[idx]].b_encoded_bits_len == 0) {
					// if no bit change (prev tail ~ cur head) & encoded bits==0x00 or 0xFF, accum tail run length
					qw_prev_tail_len += st1d_ENCODING_TABLE[rd_buffer[idx]].b_tail_run_len;
					continue;
				}
				// if no bit change & encoded bits!=0x00 or 0xFF, add prev_tail_len & current head_bits_len.
				qw_prev_tail_len += st1d_ENCODING_TABLE[rd_buffer[idx]].b_head_run_len;
				// remove encoded head bits from encoding table's encoded bits. 
				if(rd_buffer[idx] >> 7) {
					w_head_bits_len = st1d_ENCODING_TABLE[rd_buffer[idx]].b_head_run_len; // head_bits == 1-run. 
				} else {
					if(st1d_ENCODING_TABLE[rd_buffer[idx]].b_head_run_len > 2) {
						w_head_bits_len = (LOG2(st1d_ENCODING_TABLE[rd_buffer[idx]].b_head_run_len - 1) << 1) - 1; // head_bits == 0-run (longer than 2).
					} else w_head_bits_len = 2; // head_bits == 0-run (length 1 or 2).
				}
				w_enc_bits = st1d_ENCODING_TABLE[rd_buffer[idx]].w_encoded_bits << w_head_bits_len;
				w_enc_bits_len = (word)st1d_ENCODING_TABLE[rd_buffer[idx]].b_encoded_bits_len - w_head_bits_len;
				
			} else {
				w_enc_bits = st1d_ENCODING_TABLE[rd_buffer[idx]].w_encoded_bits;
				w_enc_bits_len = (word)st1d_ENCODING_TABLE[rd_buffer[idx]].b_encoded_bits_len;
			}
			// write encoded prev tail run & encoding table's encoded bits.
			fill_buf_and_wr_file(&b_prev_tail_bit, &qw_prev_tail_len, w_enc_bits, w_enc_bits_len);
			
			// reset prev_tail_bit & qw_prev_tail_len.
			b_prev_tail_bit = rd_buffer[idx] & 0b1;
			qw_prev_tail_len = st1d_ENCODING_TABLE[rd_buffer[idx]].b_tail_run_len;
		}
		
		// update progress bar.
		draw_progress_bar(fp_wrb_input_file);
	}
	// write rest encoded bits. (last write)
	fill_buf_and_wr_file(&b_prev_tail_bit, &qw_prev_tail_len, 0b1, 0);
	
	fflush(fp_wrb_output_file);
	
	return;
}
