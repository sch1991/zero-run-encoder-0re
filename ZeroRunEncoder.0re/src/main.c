#include "main.h"

Config st_CONFIG;
Resources st_RESOURCES;

int main(const int argc, const char* argv[]) {
	
	// register signal hander.
	signal(SIGINT, sig_cleanup_handler);
	signal(SIGSEGV, sig_cleanup_handler);
	signal(SIGTERM, sig_cleanup_handler);
	
	char* cp_file_path;
	
	Key* stp_key;
	byte b_head_bits;
	
	word idx;
	word temp_idx = 0;
	
	FILE* fp_input_file;
	FILE* fp1d_key_file[2];
	FILE* fp1d_temp_file[2];
	FILE* fp_output_file;
	
	if(argc > 1) {
		
		const char* cp_mode = argv[1];
		
		// init st_CONFIG.
		if(load_config() < 0) cleanup_resources(-1, "> Failed to load configuration\n"); // exit(-1);
		
		// alloc memory for stp_key.
		if((stp_key = (Key*)malloc(sizeof(Key))) == NULL) cleanup_resources(-1, "> Failed to allocate memory for key"); // exit(-1);
			st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = stp_key;
		
		switch(argc) {
			
			case 4:
				
				// verify directory structure & create directories.
				if(access(TEMP_DIR_PATH, F_OK) != 0 && mkdir(TEMP_DIR_PATH, 0755) != 0) cleanup_resources(-1, "> Failed to create temp directory"); // exit(-1);
				if(access(BIN_DIR_PATH, F_OK) != 0 && mkdir(BIN_DIR_PATH, 0755) != 0) cleanup_resources(-1, "> Failed to create bin directory"); // exit(-1);
				
				// create key0 file. (temp file)
				if((fp1d_key_file[0] = fopen(KEY_PATH(0), "wb+")) == NULL) cleanup_resources(-1, "> Failed to create key0 file"); // exit(-1);
					st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp1d_key_file[0];
					st_RESOURCES.cp1d_path_stack[st_RESOURCES.w_path_stack_top++] = KEY_PATH(0);
				// create key1 file. (temp file)
				if((fp1d_key_file[1] = fopen(KEY_PATH(1), "wb+")) == NULL) cleanup_resources(-1, "> Failed to create key1 file"); // exit(-1);
					st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp1d_key_file[1];
					st_RESOURCES.cp1d_path_stack[st_RESOURCES.w_path_stack_top++] = KEY_PATH(1);
				// create tmp0 file. (temp file)
				if((fp1d_temp_file[0] = fopen(TEMP_PATH(0), "wb+")) == NULL) cleanup_resources(-1, "> Failed to create tmp0 file"); // exit(-1);
					st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp1d_temp_file[0];
					st_RESOURCES.cp1d_path_stack[st_RESOURCES.w_path_stack_top++] = TEMP_PATH(0);
				// create tmp1 file. (temp file)
				if((fp1d_temp_file[1] = fopen(TEMP_PATH(1), "wb+")) == NULL) cleanup_resources(-1, "> Failed to create tmp1 file"); // exit(-1);
					st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp1d_temp_file[1];
					st_RESOURCES.cp1d_path_stack[st_RESOURCES.w_path_stack_top++] = TEMP_PATH(1);
				
				if(strcmp(cp_mode, ENCODING_MODE) == 0) {
					
					printf("CONFIG.ENCODING_COUNT: %zu\n" "CONFIG.BUFFER_SIZE: %.2lf %s\n" "CONFIG.CHUNK_SIZE: %zu Byte\n" "CONFIG.USE_MAPPING_TABLE_SET: %s\n" "CONFIG.USE_CONV_TABLE_SET: %s\n", 
						st_CONFIG.w_encoding_cnt, (double)st_CONFIG.w_buffer_size/(st_CONFIG.w_buffer_size<(0b1<<MiB_OFFSET) ? 0b1<<KiB_OFFSET : 0b1<<MiB_OFFSET), (st_CONFIG.w_buffer_size<(0b1<<MiB_OFFSET) ? "KiB" : "MiB"), 
						st_CONFIG.w_chunk_size, (st_CONFIG.use_mapping_table_set ? "true" : "false"), (st_CONFIG.use_conv_table_set ? "true" : "false"));
					
					const char* cp_input_file_path = argv[2];
					const char* cp_output_file_path = argv[3];
					
					char* cp_file_ext;
					const char* cp_file_name;
					
					word w_file_name_len;
					
					printf("<< Program starts in encoding mode >>\n" "input_file_path: \"%s\"\n", cp_input_file_path);
					
					// open source file.
					if((fp_input_file = fopen(cp_input_file_path, "rb")) == NULL) cleanup_resources(-1, "> Failed to open source file"); // exit(-1);
						st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_input_file;
					
					// make 0re file path.
					cp_file_path = malloc((strlen(cp_output_file_path) + 5) * sizeof(char)); // 5 == string length of ".0re\0".
					if(cp_file_path == NULL) cleanup_resources(-1, "> Failed to allocate memory for 0re_file_path"); // exit(-1);
						st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = cp_file_path;
					if((cp_file_name = strrchr(cp_output_file_path, PS[0])) == NULL)
						cp_file_name = cp_output_file_path;
					cp_file_ext = strrchr(cp_file_name, '.');
					if(cp_file_ext == NULL || strcmp(cp_file_ext, ".0re") != 0) {
						sprintf(cp_file_path, "%s.0re", cp_output_file_path);
					} else strcpy(cp_file_path, cp_output_file_path);
					
					// check for duplicate & generate unique 0re file name. 
					if(access(cp_file_path, F_OK) == 0) {
						printf("> 0re file already exist.\n" "0re_file_path: \"%s\"\n", cp_file_path);
						realloc(cp_file_path, (strlen(cp_file_path) + 3 + snprintf(NULL, 0, "%d", MAX_DUP_NAME)) * sizeof(char)); // 3 == string length of " ()", snprintf(NULL, 0, ...) == string length of MAX_DUP_NAME.
						if(cp_file_path == NULL) cleanup_resources(-1, "> Failed to reallocate memory for 0re_file_path"); // exit(-1);
							st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top-1] = cp_file_path; // must follow after malloc for cp_file_path.
						gen_uniq_file_name(cp_file_path);
					}
					
					// create 0re file.
					if((fp_output_file = fopen(cp_file_path, "wb")) == NULL) cleanup_resources(-1, "> Faild to create 0re file"); // exit(-1);
						st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_output_file;
						st_RESOURCES.cp1d_path_stack[st_RESOURCES.w_path_stack_top++] = cp_file_path;
					
					// init stp_key.
					cp_file_name = strrchr(cp_input_file_path, PS[0]);
					cp_file_name = (cp_file_name != NULL) ? cp_file_name+1 : cp_input_file_path;
					w_file_name_len = strlen(cp_file_name);
					if(w_file_name_len > NAME_MAX) cleanup_resources(-1, "> File name is too long. name_len: %zu, NAME_MAX: %d\n", w_file_name_len, NAME_MAX); // exit(-1);
					init_struct_key(stp_key, fp_input_file, (byte)w_file_name_len, cp_file_name, fp1d_key_file);
					
					// convert source file.
					wr_converted_file(true, fp_input_file, fp1d_temp_file[temp_idx], stp_key->fp_map_chg_rec_file, stp_key->fp_chunk_maps_file);
					
					// prepare encoding.
					if(load_encoding_table() < 0) cleanup_resources(-1, "> Failed to load encoding table\n"); // exit(-1);
					
					// process encoding.
					for(idx=0 ; idx<st_CONFIG.w_encoding_cnt ; idx++) {
						b_head_bits = extr_head_bits(fp1d_temp_file[temp_idx]);
						add_head_bits(&stp_key->b_key_head, b_head_bits);
						
						wr_encoded_file(true, fp1d_temp_file[temp_idx], fp1d_temp_file[!temp_idx]);
						temp_idx = !temp_idx;
					}
					
					// write 0re file.
					wr_0re_file(stp_key, fp1d_temp_file[temp_idx], fp_output_file);
					
					printf("> Encoded 0re file created successfully.\n" "0re_file_path: \"%s\"\n", cp_file_path);
						
				} else if(strcmp(cp_mode, DECODING_MODE) == 0) {
					
					printf("CONFIG.BUFFER_SIZE: %.2lf %s\n" "CONFIG.USE_MAPPING_TABLE_SET: %s\n" "CONFIG.USE_CONV_TABLE_SET: %s\n", 
						(double)st_CONFIG.w_buffer_size/(st_CONFIG.w_buffer_size<(0b1<<MiB_OFFSET) ? 0b1<<KiB_OFFSET : 0b1<<MiB_OFFSET), (st_CONFIG.w_buffer_size<(0b1<<MiB_OFFSET) ? "KiB" : "MiB"), 
						(st_CONFIG.use_mapping_table_set ? "true" : "false"), (st_CONFIG.use_conv_table_set ? "true" : "false"));
						
					const char* cp_0re_file_path = argv[2];
					const char* cp_output_dir_path = argv[3];
					
					word w_dir_path_len;
					qword qw_dec_file_size;
					
					printf("<< Program starts in decoding mode >>\n" "0re_file_path: \"%s\"\n", cp_0re_file_path);
					
					// open 0re file.
					if((fp_input_file = fopen(cp_0re_file_path, "rb")) == NULL) cleanup_resources(-1, "> Failed to open 0re file"); // exit(-1);
						st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_input_file;
					
					// read 0re file to separate key & encoded data.
					rd_0re_file(stp_key, fp_input_file, fp1d_key_file, fp1d_temp_file[temp_idx]);
					
					printf("Key.Encoding_Count: %zu\n" "Key.Output_File_Name: \'%s\'\n" "Key.Output_File_Size: %.2lf %s\n" "Key.Chunk_Size: %zu Byte\n", st_CONFIG.w_encoding_cnt, stp_key->cp_src_file_name, 
						(double)stp_key->qw_src_file_size/(stp_key->qw_src_file_size<(0b1<<MiB_OFFSET) ? (stp_key->qw_src_file_size<(0b1<<KiB_OFFSET) ? 0b1 : 0b1<<KiB_OFFSET) : 0b1<<MiB_OFFSET), 
						(stp_key->qw_src_file_size<(0b1<<MiB_OFFSET) ? (stp_key->qw_src_file_size<(0b1<<KiB_OFFSET) ? "Byte" : "KiB") : "MiB"), st_CONFIG.w_chunk_size);
					
					// make output file path.
					w_dir_path_len = strlen(cp_output_dir_path);
					cp_file_path = malloc((w_dir_path_len + 1 + stp_key->b_src_file_name_len + 1) * sizeof(char)); // 1 == string length of 'PS[0]', '\0'.
					if(cp_file_path == NULL) cleanup_resources(-1, "> Failed to allocate memory for output_file_path"); // exit(-1);
						st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = cp_file_path;
					if(cp_output_dir_path[w_dir_path_len-1] != PS[0]) {
						sprintf(cp_file_path, "%s%c%s", cp_output_dir_path, PS[0], stp_key->cp_src_file_name);
					} else sprintf(cp_file_path, "%s%s", cp_output_dir_path, stp_key->cp_src_file_name);
					
					// check for duplicate & generate unique output file name.
					if(access(cp_file_path, F_OK) == 0) {
						printf("> Output file already exist.\n" "output_file_path: \"%s\"\n", cp_file_path);
						realloc(cp_file_path, (strlen(cp_file_path) + 3 + snprintf(NULL, 0, "%d", MAX_DUP_NAME)) * sizeof(char)); // 3 == string length of " ()", snprintf(NULL, 0, ...) == string length of MAX_DUP_NAME.
						if(cp_file_path == NULL) cleanup_resources(-1, "> Failed to reallocate memory for 0re_file_path"); // exit(-1);
							st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top-1] = cp_file_path; // must follow after malloc for cp_file_path.
						gen_uniq_file_name(cp_file_path);
					}
					
					// create output file.
					if((fp_output_file = fopen(cp_file_path, "wb")) == NULL) cleanup_resources(-1, "> Failed to create output file"); // exit(-1);
						st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_output_file;
						st_RESOURCES.cp1d_path_stack[st_RESOURCES.w_path_stack_top++] = cp_file_path;
					
					// process decoding.
					for(idx=0b11-st_CONFIG.w_encoding_cnt ; idx<0b11 ; idx++) {
						qw_dec_file_size = wr_decoded_file(0, fp1d_temp_file[temp_idx], fp1d_temp_file[!temp_idx]);
						if(qw_dec_file_size != ftello(fp1d_temp_file[!temp_idx])) cleanup_resources(-1, "> Failed to decode 0re file: Thread sync error\n"); // exit(-1);
						temp_idx = !temp_idx;
						
						b_head_bits = ((stp_key->b_key_head >> (idx << 1)) & 0b11) << 6;
						set_head_bits(b_head_bits, fp1d_temp_file[temp_idx]);
					}
					
					// check 0re file corruption.
					if(st_CONFIG.w_encoding_cnt == 0) qw_dec_file_size = ftello(fp1d_temp_file[temp_idx]); // fp1d_temp_file[temp_idx]'s pos == EOF.
					if(qw_dec_file_size != stp_key->qw_src_file_size) cleanup_resources(-1, "> Corrupted 0re file: Source file and decoded file mismatch.\n" "src_file_size: %llu, dec_file_size: %llu\n", stp_key->qw_src_file_size, qw_dec_file_size); // exit(-1);
					
					// revert to original file.
					wr_converted_file(false, fp1d_temp_file[temp_idx], fp_output_file, stp_key->fp_map_chg_rec_file, stp_key->fp_chunk_maps_file);
					
					printf("> Decoded original file created successfully.\n" "output_file_path: \"%s\"\n", cp_file_path);
					
				} else cleanup_resources(-1, "> Invalid mode argument. mode: \'%s\'\n", cp_mode); // exit(-1);
				
				st_RESOURCES.w_path_stack_top--; // pop output file path. (not to be deleted.)
				
				printf(">> Program completed in %s mode <<\n", cp_mode);
				
				break;
			
			case 3:
				
				if(strcmp(cp_mode, SHOW_DETAILS_MODE) == 0) {
					
					const char* cp_0re_file_path = argv[2];
					
					uint32_t u_0re_file_header;
					
					printf("<< Display 0re file details >>\n" "0re_file_path: \"%s\"\n", cp_0re_file_path);
					
					// open 0re file.
					if((fp_input_file = fopen(cp_0re_file_path, "rb")) == NULL) cleanup_resources(-1, "> Failed to open 0re file"); // exit(-1)
						st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_input_file;
					
					// read & check 0re file's signature.
					fread(&u_0re_file_header, sizeof(uint32_t), 1, fp_input_file);
					if(u_0re_file_header != ZERORE_SIGNATURE) cleanup_resources(-1, "> File is not 0re file: File signature mismatch\n"); // exit(-1);
					
					// read key data from 0re file.
					rd_key_from_0re(stp_key, fp_input_file, NULL);
					
					// show 0re file details.
					printf("Encoding_Count: %zu\n" "Original_File_Name: \'%s\'\n" "Original_File_Size: %.2lf %s\n" "Chunk_Size: %zu Byte\n", st_CONFIG.w_encoding_cnt, stp_key->cp_src_file_name, 
						(double)stp_key->qw_src_file_size/(stp_key->qw_src_file_size<(0b1<<MiB_OFFSET) ? (stp_key->qw_src_file_size<(0b1<<KiB_OFFSET) ? 0b1 : 0b1<<KiB_OFFSET) : 0b1<<MiB_OFFSET), 
						(stp_key->qw_src_file_size<(0b1<<MiB_OFFSET) ? (stp_key->qw_src_file_size<(0b1<<KiB_OFFSET) ? "Byte" : "KiB") : "MiB"), st_CONFIG.w_chunk_size);
						
				} else cleanup_resources(-1, "> Invalid mode argument. mode: \'%s\'\n", cp_mode); // exit(-1);
				
				break;
				
			default:
				cleanup_resources(-1, "> Invalid number of arguments. argc: %d\n", argc); // exit(-1);
		}
		
	} else cleanup_resources(-1, "> Too few arguments, at least 2 required. argc: %d\n", argc); // exit(-1);

	cleanup_resources(0, NULL); // exit(0);
}



static inline void gen_uniq_file_name(char* cp_file_path) {
	
	word idx;
	
	char* cp_base_file_path, * cp_base_file_name, * cp_base_file_ext;
	char c_ext_dot = '.';
	
	char* cp_new_file_name;
	
	// alloc memory for base file path.
	cp_base_file_path = malloc(strlen(cp_file_path) * sizeof(char));
	if(cp_base_file_path == NULL) cleanup_resources(-1, "> Failed to allocate memory for base_file_path"); // exit(-1);
		st_RESOURCES.vp1d_heap_stack[st_RESOURCES.w_heap_stack_top++] = cp_base_file_path;
	
	// separate base file path into 'file path without ext', 'dot', 'extension'.
	strcpy(cp_base_file_path, cp_file_path);
	if((cp_base_file_name = strrchr(cp_base_file_path, PS[0])) == NULL)
		cp_base_file_name = cp_base_file_path;
	cp_base_file_ext = strrchr(cp_base_file_name, '.');
	if(cp_base_file_ext != NULL) *(cp_base_file_ext++) = '\0';
	else {
		// if no extension, set dot & extension to '\0'. 
		c_ext_dot = '\0';
		cp_base_file_ext = &c_ext_dot;
	} 
	
	// generate unique file name.
	for(idx=2 ; idx<=MAX_DUP_NAME ; idx++) {
		sprintf(cp_file_path, "%s (%d)%c%s", cp_base_file_path, idx, c_ext_dot, cp_base_file_ext);
		if(access(cp_file_path, F_OK) != 0) break;
	}
	
	if(idx > MAX_DUP_NAME) cleanup_resources(-1, "> Duplicate file names exceed limit. MAX_DUP_NAME: %d\n", MAX_DUP_NAME); // exit(-1);
	
	cp_new_file_name = strrchr(cp_file_path, PS[0]);
	cp_new_file_name = (cp_new_file_name != NULL) ? cp_new_file_name+1 : cp_file_path;
	printf("> New file name generated. new_file_name: \'%s\'\n", cp_new_file_name);
	
	free(st_RESOURCES.vp1d_heap_stack[--st_RESOURCES.w_heap_stack_top]); // free(cp_base_file_path);
	
	return;
}



static inline void wr_0re_file(Key* stp_key, FILE* fp_wrb_encoded_file, FILE* fp_wb_0re_file) {
	
	puts("> Encoded 0re file creation in progress...");
	
	// reset 0re file pointer & truncate 0re files to 0 byte.
	fseek(fp_wb_0re_file, 0, SEEK_SET);
	ftruncate(fileno(fp_wb_0re_file), 0);
	
	uint32_t u_0re_file_header = ZERORE_SIGNATURE;
	
	// write 0re file's signature.
	fwrite(&u_0re_file_header, sizeof(uint32_t), 1, fp_wb_0re_file);

	// write key data into 0re file.
	wr_key_to_0re(stp_key, fp_wb_0re_file);
	
	word w_read_len;
	byte buffer[st_CONFIG.w_buffer_size];
	
	// reset encoded file pointer & init progress bar.
	fseek(fp_wrb_encoded_file, 0, SEEK_SET);
	draw_progress_bar(fp_wrb_encoded_file);
	
	// write encoded file data into 0re file.
	while((w_read_len = fread(buffer, sizeof(byte), sizeof(buffer), fp_wrb_encoded_file)) > 0) {
		fwrite(buffer, sizeof(byte), w_read_len, fp_wb_0re_file);
		draw_progress_bar(fp_wrb_encoded_file); // update progress bar.
	}
	
	fflush(fp_wb_0re_file);
	
	return;
}


 
static inline void rd_0re_file(Key* stp_key, FILE* fp_rb_0re_file, FILE* fp1d_wrb_key_file[2], FILE* fp_wrb_temp_file) {
	
	puts("> Key & encoded data separation from 0re file in progress...");
	
	// reset 0re file pointer & init progress bar.
	fseek(fp_rb_0re_file, 0, SEEK_SET);
	draw_progress_bar(fp_rb_0re_file);
	
	uint32_t u_0re_file_header;
	
	// read & check 0re file's signature.
	fread(&u_0re_file_header, sizeof(uint32_t), 1, fp_rb_0re_file);
	if(u_0re_file_header != ZERORE_SIGNATURE) cleanup_resources(-1, "\r> File is not 0re file: File signature mismatch\n"); // exit(-1);
	
	// read key data from 0re file.
	rd_key_from_0re(stp_key, fp_rb_0re_file, fp1d_wrb_key_file);
	
	word w_read_len;
	byte buffer[st_CONFIG.w_buffer_size];
	
	// reset temp file pointer & truncate temp file to 0 byte.
	fseek(fp_wrb_temp_file, 0, SEEK_SET);
	ftruncate(fileno(fp_wrb_temp_file), 0);
	
	// separate encoded data & write into temp file.
	while((w_read_len = fread(buffer, sizeof(byte), sizeof(buffer), fp_rb_0re_file)) > 0) {
		fwrite(buffer, sizeof(byte), w_read_len, fp_wrb_temp_file);
		draw_progress_bar(fp_rb_0re_file); // update progress bar.
	}
	
	fflush(fp_wrb_temp_file);
	
	return;
}



static void sig_cleanup_handler(int sig) {
	
	if(sig > 0) cleanup_resources(-1, "> Terminate process due to signal. signal_code: 0x%02X\n", sig); // exit(-1);
	
	cleanup_resources(0, NULL); // exit(0);
}
