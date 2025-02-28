#include "keymaker.h"

void init_struct_key(Key* stp_key, FILE* fp_rb_src_file, byte b_src_file_name_len, const char* cp_src_file_name, FILE* fp1d_wrb_key_file[2]) {
	
	// set stp_key->b_key_head.
	stp_key->b_key_head = 0b0;
	
	// set stp_key->cp_src_file_name_len & stp_key->cp_src_file_name.
	stp_key->b_src_file_name_len = b_src_file_name_len;
	stp_key->cp_src_file_name = cp_src_file_name;
	
	// set stp_key->qw_src_file_size.
	fseeko(fp_rb_src_file, 0, SEEK_END);
	stp_key->qw_src_file_size = ftello(fp_rb_src_file);
	fseeko(fp_rb_src_file, 0, SEEK_SET);
	
	// set stp_key->b_chunk_size
	stp_key->b_chunk_size = (byte)(((st_CONFIG.w_chunk_size >> st_CONFIG.w_chunk_offset) - 1)<<3 | st_CONFIG.w_chunk_offset);
	
	// set stp_key->fp_map_chg_rec_file & stp_key->fp_chunk_maps_file.
	stp_key->fp_map_chg_rec_file = fp1d_wrb_key_file[0];
	stp_key->fp_chunk_maps_file = fp1d_wrb_key_file[1];
	// write fp_map_chg_rec_file & fp_chunk_maps_file.
	wr_key_files(fp_rb_src_file, stp_key->fp_map_chg_rec_file, stp_key->fp_chunk_maps_file);
	
	return;
}



static void wr_key_files(FILE* fp_rb_src_file, FILE* fp_wrb_map_chg_rec_file, FILE* fp_wrb_chunk_maps_file) {

	word idx;
	word w_read_len;
	byte rd_buffer[st_CONFIG.w_buffer_size];
	byte wr_rec_buffer[st_CONFIG.w_buffer_size>>3];
	byte wr_map_buffer[st_CONFIG.w_buffer_size];
	
	bool is_src_head = true;
	byte b1d_src_bits[4];
	byte b_prev_bits;
	
	bool is_rec_head = true;	
	byte b_freq_code, b_chunk_map, b_cur_map;
	word w_rd_buf_cnt, w_rec_bit_cnt, w_map_cnt; // (rec_bit_cnt >> 3) : rec_buf_idx, (rec_bit_cnt & 0b111) : bit_cnt in current rec_buf.
	
	qword qw2d_condi_prob[4][4] = {0, }; // condi_prob : frequent of each 2-bit pattern following 2-bit codes.
	
	
	// define 'write map change record file & chunk's maps file' inner function. (is_last_wr == true : write even if buffers not full.) 
	void wr_rec_and_maps(bool is_last_wr) {
		
		word inner_idx;
		
		if(w_rd_buf_cnt > 0) {	
			// make chunk's map. (0b(00->)**(01->)**(10->)**(11->)**)
			for(b_chunk_map=0b0, b_freq_code=0b00, inner_idx=0b00 ; inner_idx<=0b11 ; inner_idx++, b_freq_code=0b00) {
				if(qw2d_condi_prob[inner_idx][b_freq_code] < qw2d_condi_prob[inner_idx][0b01]) b_freq_code = 0b01;
				if(qw2d_condi_prob[inner_idx][b_freq_code] < qw2d_condi_prob[inner_idx][0b10]) b_freq_code = 0b10;
				if(qw2d_condi_prob[inner_idx][b_freq_code] < qw2d_condi_prob[inner_idx][0b11]) b_freq_code = 0b11;
				
				b_chunk_map |= b_freq_code << (0b11-inner_idx << 1);
			}
			
			if(is_rec_head) {
				b_cur_map = ~b_chunk_map; // for (b_cur_map != b_chunk_map) == true.
				is_rec_head = !is_rec_head;
			}
			
			// if map changed, write map to wr_map_buffer & record 0b0 in wr_rec_buffer. (just w_rec_bit_cnt++)
			if(b_cur_map != b_chunk_map) {
				wr_map_buffer[w_map_cnt++] = b_chunk_map;
				b_cur_map = b_chunk_map;
			} else wr_rec_buffer[w_rec_bit_cnt>>3] |= 0b1 << (~w_rec_bit_cnt&0b111); // record 0b1 in wr_rec_buffer.
			w_rec_bit_cnt++;
		}
		
		// write wr_rec_buffer data into map change record file.
		if(w_rec_bit_cnt>>3 == sizeof(wr_rec_buffer) || is_last_wr) {
			fwrite(wr_rec_buffer, sizeof(byte), (w_rec_bit_cnt+0b111)>>3, fp_wrb_map_chg_rec_file);
			memset(wr_rec_buffer, 0x00, sizeof(wr_rec_buffer));
			w_rec_bit_cnt = 0;
		}
		
		// write wr_map_buffer data into chunk's maps file.
		if(w_map_cnt == sizeof(wr_map_buffer) || is_last_wr) {
			fwrite(wr_map_buffer, sizeof(byte), w_map_cnt, fp_wrb_chunk_maps_file);
			w_map_cnt = 0;
		}
		
		w_rd_buf_cnt = 0;
		memset(qw2d_condi_prob, 0, sizeof(qw2d_condi_prob));
		
		return;
	}
	
	
	puts("> Key initialization in progress...");
	
	// reset read file pointer & init progress bar.
	fseek(fp_rb_src_file, 0, SEEK_SET);
	draw_progress_bar(fp_rb_src_file);
	
	// reset write file pointers & truncate output files to 0 byte.
	fseek(fp_wrb_map_chg_rec_file, 0, SEEK_SET);
	ftruncate(fileno(fp_wrb_map_chg_rec_file), 0);
	fseek(fp_wrb_chunk_maps_file, 0, SEEK_SET);
	ftruncate(fileno(fp_wrb_chunk_maps_file), 0);
	
	memset(wr_rec_buffer, 0x00, sizeof(wr_rec_buffer));
	w_rd_buf_cnt = w_rec_bit_cnt = w_map_cnt = 0;
	
	while((w_read_len = fread(rd_buffer, sizeof(byte), sizeof(rd_buffer), fp_rb_src_file)) > 0) {
		
		for(idx=0 ; idx<w_read_len ; idx++) {
			// split src bits.
			b1d_src_bits[0] = rd_buffer[idx] >> 6;
			b1d_src_bits[1] = (rd_buffer[idx] >> 4) & 0b11;
			b1d_src_bits[2] = (rd_buffer[idx] >> 2) & 0b11;
			b1d_src_bits[3] = rd_buffer[idx] & 0b11;
			
			// count next 2-bit pattern freq.
			if(!is_src_head) {
				qw2d_condi_prob[b_prev_bits][b1d_src_bits[0]]++;
			} else is_src_head = !is_src_head;
			qw2d_condi_prob[b1d_src_bits[0]][b1d_src_bits[1]]++;
			qw2d_condi_prob[b1d_src_bits[1]][b1d_src_bits[2]]++;
			qw2d_condi_prob[b1d_src_bits[2]][b1d_src_bits[3]]++;
			
			b_prev_bits = b1d_src_bits[3];

			// write map change record & chunk's maps.
			if(++w_rd_buf_cnt == st_CONFIG.w_chunk_size) wr_rec_and_maps(false);
		}
		
		// update progress bar.
		draw_progress_bar(fp_rb_src_file);
	}
	// write last map change record & chunk's maps.
	wr_rec_and_maps(true);
	
	fflush(fp_wrb_map_chg_rec_file);
	fflush(fp_wrb_chunk_maps_file);
	
	return;
}



void add_head_bits(byte* bp_key_head, byte b_head_bits) {
	
	byte b_enc_cnt = *bp_key_head >> 6;
	
	// increase cnt & store head bits. (b_head_bits : 0b**000000)
	if(b_enc_cnt++ < 0b11) {
		*bp_key_head += 0b01000000;
		*bp_key_head |= b_head_bits >> (b_enc_cnt<<1);
	} else cleanup_resources(-1, "> Exceeded maximum encoding count\n"); // exit(-1);
	
	return;
}



void wr_key_to_0re(Key* stp_key, FILE* fp_wb_0re_file) {
	
	word w_read_len;
	byte buffer[st_CONFIG.w_buffer_size];
	
	// write key head.
	fwrite(&stp_key->b_key_head, sizeof(byte), 1, fp_wb_0re_file);
	
	// write source file's name length & name. (excluding '\0')
	fwrite(&stp_key->b_src_file_name_len, sizeof(byte), 1, fp_wb_0re_file);
	fwrite(stp_key->cp_src_file_name, sizeof(char), stp_key->b_src_file_name_len, fp_wb_0re_file);
	
	// write source file size & chunk size.
	fwrite(&stp_key->qw_src_file_size, sizeof(qword), 1, fp_wb_0re_file);
	fwrite(&stp_key->b_chunk_size, sizeof(byte), 1, fp_wb_0re_file);
	
	// encode & write map_chg_rec_file data.
	wr_encoded_file(false, stp_key->fp_map_chg_rec_file, fp_wb_0re_file);
	
	// reset chunk_maps_file pointer & init progress bar.
	fseek(stp_key->fp_chunk_maps_file, 0, SEEK_SET);
	draw_progress_bar(stp_key->fp_chunk_maps_file);
	// write chunk_maps_file data.
	while((w_read_len = fread(buffer, sizeof(byte), sizeof(buffer), stp_key->fp_chunk_maps_file)) > 0) {
		fwrite(buffer, sizeof(byte), w_read_len, fp_wb_0re_file);
		draw_progress_bar(stp_key->fp_chunk_maps_file); // update progress bar.
	}
	
	fflush(fp_wb_0re_file);
	
	return;
}



void rd_key_from_0re(Key* stp_key, FILE* fp_rb_0re_file, FILE* fp1d_wrb_key_file[2]) {
	
	word w_read_len;
	byte buffer[st_CONFIG.w_buffer_size];
	
	char* cp_file_name;
	
	// read & set stp_key->b_key_head(st_CONFIG.w_encoding_cnt).
	fread(&stp_key->b_key_head, sizeof(byte), 1, fp_rb_0re_file);
	st_CONFIG.w_encoding_cnt = stp_key->b_key_head >> 6; // get encoding count & set st_CONFIG.w_encoding_cnt for decoding.
	
	// read & set stp_key->cp_src_file_name_len & stp_key->cp_src_file_name.
	fread(&stp_key->b_src_file_name_len, sizeof(byte), 1, fp_rb_0re_file);
	cp_file_name = malloc((stp_key->b_src_file_name_len + 1) * sizeof(char)); // 1 == '\0'
	if(cp_file_name == NULL) cleanup_resources(-1, "> Failed to allocate memory for output_file_name"); // exit(-1);
		st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = cp_file_name;
	fread(cp_file_name, sizeof(char), stp_key->b_src_file_name_len, fp_rb_0re_file);
	cp_file_name[stp_key->b_src_file_name_len] = '\0';
	stp_key->cp_src_file_name = cp_file_name;
	
	// read & set stp_key->qw_src_file_size & stp_key->b_chunk_size(st_CONFIG.w_chunk_size).
	fread(&stp_key->qw_src_file_size, sizeof(qword), 1, fp_rb_0re_file);
	if(st_CONFIG.use_file_size_limit && stp_key->qw_src_file_size > (word)(~0b0)) // check file system error. (if use file_size_limit.)
		cleanup_resources(-1, "> Output file size exceeds limit. file_size: %.2lf GiB\n" "(To remove the limit, set CONFIG.USE_FILE_SIZE_LIMIT to \"false\")\n", 
			(double)stp_key->qw_src_file_size / (0b1<<30)); // exit(-1);
	fread(&stp_key->b_chunk_size, sizeof(byte), 1, fp_rb_0re_file);
	st_CONFIG.w_chunk_offset = stp_key->b_chunk_size & 0b111;
	st_CONFIG.w_chunk_size = (((word)stp_key->b_chunk_size>>3) + 1) << st_CONFIG.w_chunk_offset; // set st_CONFIG.w_chunk_size for reverting.
	
	if(fp1d_wrb_key_file != NULL) {
		
		qword qw_rec_bits_len, qw_rec_bits_len_b_pad;
		qword qw_chunk_map_rest_len;
		
		// read & write stp_key->fp_map_chg_rec_file.
		stp_key->fp_map_chg_rec_file = fp1d_wrb_key_file[0];
		qw_rec_bits_len = (stp_key->qw_src_file_size+st_CONFIG.w_chunk_size-1) / (qword)st_CONFIG.w_chunk_size; // round up 'src_size/chunk_size'.
		qw_rec_bits_len_b_pad = (8 - (qw_rec_bits_len & 0b111)) & 0b111; // b_pad : byte padding for qw_rec_bits_len & qw_chunk_map_rest_len.
		qw_chunk_map_rest_len = wr_decoded_file(qw_rec_bits_len+qw_rec_bits_len_b_pad, fp_rb_0re_file, stp_key->fp_map_chg_rec_file) - qw_rec_bits_len_b_pad;
		
		// read & write stp_key->fp_chunk_maps_file.
		stp_key->fp_chunk_maps_file = fp1d_wrb_key_file[1];
		fseek(stp_key->fp_chunk_maps_file, 0, SEEK_SET); // reset chunk_maps_file pointer.
		ftruncate(fileno(stp_key->fp_chunk_maps_file), 0); // truncate chunk_maps_file to 0 byte.
		while(qw_chunk_map_rest_len > sizeof(buffer)) {
			w_read_len = fread(buffer, sizeof(byte), sizeof(buffer), fp_rb_0re_file);
			fwrite(buffer, sizeof(byte), w_read_len, stp_key->fp_chunk_maps_file);
			draw_progress_bar(fp_rb_0re_file); // update progress bar.
			
			qw_chunk_map_rest_len -= sizeof(buffer);
		}
		w_read_len = fread(buffer, sizeof(byte), qw_chunk_map_rest_len, fp_rb_0re_file); // last read.
		fwrite(buffer, sizeof(byte), w_read_len, stp_key->fp_chunk_maps_file); // last write.
		draw_progress_bar(fp_rb_0re_file); // update progress bar.
		
		fflush(stp_key->fp_chunk_maps_file);
	}
	
	return;
}
