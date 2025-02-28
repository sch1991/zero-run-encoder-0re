#include "utils.h"

int load_config() {
	
	FILE* fp_config_file;
	
	if(access(CONFIG_PATH, F_OK) == 0) {
		if((fp_config_file = fopen(CONFIG_PATH, "r")) == NULL) {
			perror("> Failed to open config file");
			return -1;
		}	st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_config_file;
		
		byte buffer[CONFIG_MAX_LEN];
		char* cp_value;
		
		while(fgets(buffer, sizeof(buffer), fp_config_file)) {
			// separate key-value & remove '\n' charactor.
			cp_value = strchr(buffer, '=');
			if(cp_value != NULL) {
				*(cp_value++) = '\0';
				cp_value[strcspn(cp_value, "\r\n")] = '\0';
				
			} else continue;
			
			// get st_CONFIG values from config file.
			if(strstr(buffer, "ENCODING_COUNT") != NULL) {
				st_CONFIG.w_encoding_cnt = strtozu(cp_value, NULL, 10);
				if(st_CONFIG.w_encoding_cnt > MAX_ENCODING_COUNT) st_CONFIG.w_encoding_cnt = MAX_ENCODING_COUNT;
				else if(st_CONFIG.w_encoding_cnt < 0) st_CONFIG.w_encoding_cnt = 0;
			} else if(strstr(buffer, "BUFFER_SIZE") != NULL) {
				st_CONFIG.w_buffer_size = strtozu(cp_value, NULL, 10) << BUFFER_UNIT_OFFSET;
				if(st_CONFIG.w_buffer_size > MAX_BUFFER_SIZE) st_CONFIG.w_buffer_size = MAX_BUFFER_SIZE;
				else if(st_CONFIG.w_buffer_size < MIN_BUFFER_SIZE) st_CONFIG.w_buffer_size = MIN_BUFFER_SIZE;
			} else if(strstr(buffer, "CHUNK_SIZE") != NULL) {
				st_CONFIG.w_chunk_size = strtozu(cp_value, NULL, 10);
				if(st_CONFIG.w_chunk_size > MAX_CHUNK_SIZE) st_CONFIG.w_chunk_size = MAX_CHUNK_SIZE;
				else if(st_CONFIG.w_chunk_size < MIN_CHUNK_SIZE) st_CONFIG.w_chunk_size = MIN_CHUNK_SIZE;
			} else if(strstr(buffer, "CHUNK_OFFSET") != NULL) {
				st_CONFIG.w_chunk_offset = strtozu(cp_value, NULL, 10);
				if(st_CONFIG.w_chunk_offset > MAX_CHUNK_OFFSET) st_CONFIG.w_chunk_offset = MAX_CHUNK_OFFSET;
				else if(st_CONFIG.w_chunk_offset < MIN_CHUNK_OFFSET) st_CONFIG.w_chunk_offset = MIN_CHUNK_OFFSET;
				// scale st_CONFIG.w_chunk_size.
				if(st_CONFIG.w_chunk_size == 0) {
					puts("> CHUNK_OFFSET is set before CHUNK_SIZE"); return -1;
				} else st_CONFIG.w_chunk_size <<= st_CONFIG.w_chunk_offset;
			} else if(strstr(buffer, "USE_MAPPING_TABLE_SET") != NULL) {
				st_CONFIG.use_mapping_table_set = (strcmp(cp_value, "true") == 0);
			} else if(strstr(buffer, "USE_CONV_TABLE_SET") != NULL) {
				st_CONFIG.use_conv_table_set = (strcmp(cp_value, "true") == 0);
			} else if(strstr(buffer, "USE_FILE_SIZE_LIMIT") != NULL) {
				st_CONFIG.use_file_size_limit = (strcmp(cp_value, "true") == 0);
			}
		}
		
	} else {
		puts("> Config file not found.");
		
		if((fp_config_file = fopen(CONFIG_PATH, "w")) == NULL) {
			perror("> Failed to create config file");
			return -1;
		}	st_RESOURCES.fp1d_fp_stack[st_RESOURCES.w_fp_stack_top++] = fp_config_file;
			st_RESOURCES.cp1d_path_stack[st_RESOURCES.w_path_stack_top++] = CONFIG_PATH;
		
		// write session to config file.
		fputs("[Config]\n", fp_config_file);
		
		// set st_CONFIG.w_encoding_cnt to default value & write config file.
		st_CONFIG.w_encoding_cnt = DEFAULT_ENCODING_COUNT<=MAX_ENCODING_COUNT ? DEFAULT_ENCODING_COUNT : MAX_ENCODING_COUNT;
		fprintf(fp_config_file, "ENCODING_COUNT [Range:0~3]=%zu\n", st_CONFIG.w_encoding_cnt);
		// set st_CONFIG.w_buffer_size to default value & write config file.
		st_CONFIG.w_buffer_size = DEFAULT_BUFFER_SIZE<=MAX_BUFFER_SIZE ? DEFAULT_BUFFER_SIZE : MAX_BUFFER_SIZE;
		fprintf(fp_config_file, "BUFFER_SIZE [Unit:KiB, Range:64KiB~24MiB]=%zu\n", st_CONFIG.w_buffer_size >> BUFFER_UNIT_OFFSET);
		// set st_CONFIG.w_chunk_size to default value & write config file.
		st_CONFIG.w_chunk_size = DEFAULT_CHUNK_SIZE<=MAX_CHUNK_SIZE ? DEFAULT_CHUNK_SIZE : MAX_CHUNK_SIZE;
		fprintf(fp_config_file, "CHUNK_SIZE [Unit:Byte, Range:1Byte~32Byte]=%zu\n", st_CONFIG.w_chunk_size);
		// set st_CONFIG.w_chunk_offset to default value & write config file & scale st_CONFIG.w_chunk_size.
		st_CONFIG.w_chunk_offset = DEFAULT_CHUNK_OFFSET<=MAX_CHUNK_OFFSET ? DEFAULT_CHUNK_OFFSET : MAX_CHUNK_OFFSET;
		fprintf(fp_config_file, "CHUNK_OFFSET [Range:0~7, Chunk Size:size*(2^offset)]=%zu\n", st_CONFIG.w_chunk_offset);
		st_CONFIG.w_chunk_size <<= st_CONFIG.w_chunk_offset;
		// set st_CONFIG.use_mapping_table_set to default value & write config file.
		st_CONFIG.use_mapping_table_set = DEFAULT_USE_MAPPING_TABLE_SET;
		fprintf(fp_config_file, "USE_MAPPING_TABLE_SET [Required:4KiB Memory]=%s\n", (st_CONFIG.use_mapping_table_set ? "true" : "false"));
		// set st_CONFIG.use_conv_table_set to default value & write config file.
		st_CONFIG.use_conv_table_set = DEFAULT_USE_CONV_TABLE_SET;
		fprintf(fp_config_file, "USE_CONV_TABLE_SET [Required:64KiB Memory]=%s\n", (st_CONFIG.use_conv_table_set ? "true" : "false"));
		// set st_CONFIG.use_file_size_limit to default value & write config file.
		st_CONFIG.use_file_size_limit = DEFAULT_USE_FILE_SIZE_LIMIT;
		fprintf(fp_config_file, "USE_FILE_SIZE_LIMIT (Limit to less than 4GiB on 32-bit system.)=%s\n", (st_CONFIG.use_file_size_limit ? "true" : "false"));
		
		st_RESOURCES.w_path_stack_top--; // pop config file path. (not to be deleted.)
		
		puts("> Config file created.");
	}
	
	fclose(st_RESOURCES.fp1d_fp_stack[--st_RESOURCES.w_fp_stack_top]); // fclose(fp_config_file);

	return 0;
}



void draw_progress_bar(FILE* fp_wrb_prog_file) {
	
	static s_qword s_qw_file_size;
	static s_word s_w_prog_offset, s_w_prog_zero_base, s_w_prev_prog_pct;
	
	word idx;
	s_word s_w_cur_prog_pct;
	s_qword s_qw_cur_fp_pos;
	
	// if prog_file's pointer at 0, init function's static variables.
	if((s_qw_cur_fp_pos = ftello(fp_wrb_prog_file)) == 0) {
		// init total file size.
		fseeko(fp_wrb_prog_file, 0, SEEK_END);
		s_qw_file_size = ftello(fp_wrb_prog_file);
		fseeko(fp_wrb_prog_file, 0, SEEK_SET);
		// shift file_size's bits to keep upper 7bits and sub 99. (for displaying progress % in 0~100(=99.xx).)
		if(s_qw_file_size >= 0) {
			s_w_prog_offset = (s_word)LOG2LL(s_qw_file_size) - 7;
			if(s_w_prog_offset > 0) {
				s_w_prog_zero_base = ((qword)s_qw_file_size>>s_w_prog_offset) - 99;
			} else s_w_prog_zero_base = ((qword)s_qw_file_size<<(-s_w_prog_offset)) - 99;
			
			s_w_prev_prog_pct = 100; // to indicate initial state.
			
		} else {
			perror("> Failed to initialize progress bar");
			return;
		}
		
	} else if(s_qw_cur_fp_pos < 0) {
		perror("> Failed to initialize progress bar");
		return;
	}
	
	// calc progress %.
	if(s_w_prog_offset > 0) {
		s_w_cur_prog_pct = ((qword)s_qw_cur_fp_pos>>s_w_prog_offset) - s_w_prog_zero_base;
	} else s_w_cur_prog_pct = ((qword)s_qw_cur_fp_pos<<(-s_w_prog_offset)) - s_w_prog_zero_base;
	
	// draw progress bar.
	if(s_qw_cur_fp_pos != s_qw_file_size) {
		
		// check progress % changes.
		if(s_w_cur_prog_pct != s_w_prev_prog_pct) {
			s_w_prev_prog_pct = s_w_cur_prog_pct;
		} else return;
		
		if(s_w_cur_prog_pct < 0) {
			// (progress % < 0%) means processing data range excluded by zero_base.
			fputs("\r[", stdout);
		    printf("%*c   0%%", (99>>PROG_BAR_OFFSET)+1, ']');
		} else {
			// (progress % 0~99%) draw progress bar for prgress %.
			fputs("\r[", stdout);
		    for(idx=0 ; idx<s_w_cur_prog_pct>>PROG_BAR_OFFSET ; idx++) putchar('=');
		    printf("%*c %3d%%", (99>>PROG_BAR_OFFSET)-idx+1, ']', s_w_cur_prog_pct);
		}
		
	} else {
		// (progress 99~100%) means processing data under upper 7bits.
		fputs("\r[", stdout);
    	for(idx=0 ; idx<99>>PROG_BAR_OFFSET ; idx++) putchar('=');
    	puts("] 100%");
	}
	
    fflush(stdout);
	
	return;
}



void cleanup_resources(int exit_code, const char* cp_error_msg, ...) {
	
	// check stack overflow.
	if(st_RESOURCES.w_thrd_stack_top > THRD_STACK_SIZE) {
		printf("> Thread stack overflow has occurred. thrd_stack_top: %zu, fp_stack_size: %d\n", st_RESOURCES.w_thrd_stack_top, THRD_STACK_SIZE);
		st_RESOURCES.w_fp_stack_top = FP_STACK_SIZE;
	}
	if(st_RESOURCES.w_fp_stack_top > FP_STACK_SIZE) {
		printf("> FP stack overflow has occurred. fp_stack_top: %zu, fp_stack_size: %d\n", st_RESOURCES.w_fp_stack_top, FP_STACK_SIZE);
		st_RESOURCES.w_fp_stack_top = FP_STACK_SIZE;
	}
	if(st_RESOURCES.w_path_stack_top > PATH_STACK_SIZE) {
		printf("> Path stack overflow has occurred. path_stack_top: %zu, path_stack_size: %d\n", st_RESOURCES.w_path_stack_top, PATH_STACK_SIZE);
		st_RESOURCES.w_path_stack_top = PATH_STACK_SIZE;
	}
	if(st_RESOURCES.w_heap_stack_top > HEAP_STACK_SIZE) {
		printf("> Heap stack overflow has occurred. heap_stack_top: %zu, heap_stack_size: %d\n", st_RESOURCES.w_heap_stack_top, HEAP_STACK_SIZE);
		st_RESOURCES.w_heap_stack_top = HEAP_STACK_SIZE;
	}
	
	// print error message.
	if(cp_error_msg != NULL) {
		if(strchr(cp_error_msg, '\n') != NULL) {
			// if '\n' is in error_msg, use vprintf().
			va_list va_args;
			va_start(va_args, cp_error_msg);
			vprintf(cp_error_msg, va_args);
			va_end(va_args);
		} else {
			if(errno != 0) {
				perror(cp_error_msg); // if errno is set, use perror().
			} else puts(cp_error_msg); // otherwise, use puts().
		}
	}
	
	word w_thrd_stack_cnt = st_RESOURCES.w_thrd_stack_top;
	
	// clean up resources.
	while(st_RESOURCES.w_thrd_stack_top > 0) pthread_cancel(st_RESOURCES.thrd1d_thrd_stack[--st_RESOURCES.w_thrd_stack_top]); // send stop signal to thread.
	while(w_thrd_stack_cnt > 0) pthread_join(st_RESOURCES.thrd1d_thrd_stack[--w_thrd_stack_cnt], NULL); // wait for thread to stop.
	while(st_RESOURCES.w_fp_stack_top > 0) fclose(st_RESOURCES.fp1d_fp_stack[--st_RESOURCES.w_fp_stack_top]);
	while(st_RESOURCES.w_path_stack_top > 0) remove(st_RESOURCES.cp1d_path_stack[--st_RESOURCES.w_path_stack_top]);
	while(st_RESOURCES.w_heap_stack_top > 0) free(st_RESOURCES.vp1d_heap_stack[--st_RESOURCES.w_heap_stack_top]);
	
	exit(exit_code);
}
