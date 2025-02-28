#include "decoder.h"

Dec_Queue st_DEC_QUEUE;
Dec_Thread st_DEC_THREAD;

void* thrd_dequeue_and_wr_file(void* arg) {
	
	pthread_mutex_lock(&st_DEC_THREAD.thrd_mutex);
	
	// set variables from arguments (main thread's static variable pointers).
	word* wp_dec_cnt = ((Dec_Thrd_Arg*)arg)->wp_dec_cnt; // store decoding count's pointer.
	FILE** fpp_wrb_output_file = ((Dec_Thrd_Arg*)arg)->fpp_thrd_arg_output_file; // store output file pointer's pointer.
	
	// set thread running flag & unblock main thread's wait. 
	st_DEC_THREAD.is_running = true;
	pthread_mutex_unlock(&st_DEC_THREAD.thrd_mutex);
	pthread_cond_signal(&st_DEC_THREAD.thrd_cond);
	
	byte wr_buffer[st_CONFIG.w_buffer_size];
	
	qword qw_wr_buf_bit_cnt = 0; // (wr_bit_cnt >> 3) : wr_buf_idx, (wr_bit_cnt & 0b111) : bit_cnt in current wr_buf.
	
	
	// define 'write wr_buffer to decoded file' inner function.
	void wr_buf_to_file(word w_write_len) {
		
		do {
			// write wr_buffer data into output file.
			fwrite(wr_buffer, sizeof(byte), w_write_len, *fpp_wrb_output_file);
			
			if(w_write_len == sizeof(wr_buffer)) {
				// reduce wr_bit_cnt by written amount & reset wr_buffer.
				qw_wr_buf_bit_cnt -= (qword)sizeof(wr_buffer) << 3;
				memset(wr_buffer, 0x00, sizeof(wr_buffer));
				
			} else qw_wr_buf_bit_cnt = 0; // reset wr_bit_cnt if last write.
			
		} while(qw_wr_buf_bit_cnt>>3 >= sizeof(wr_buffer));
		
		return;
	}
	
	
	bool is_dec_0_run;
	word w_buf_rest_len;
	qword qw_dec_run_rest_len;
	
	do {
		// reset 0-run flag & wr_buffer.
		is_dec_0_run = true;
		memset(wr_buffer, 0x00, sizeof(wr_buffer));
		
		// wait until queue is filled.
		while(st_DEC_QUEUE.s_qw1d_run_len[st_DEC_QUEUE.w_front] == 0) pthread_cond_wait(&st_DEC_THREAD.thrd_cond, &st_DEC_THREAD.thrd_mutex);
		
		while(st_DEC_QUEUE.s_qw1d_run_len[st_DEC_QUEUE.w_front] > 0) {
			
			// load front element from queue.
			qw_dec_run_rest_len = st_DEC_QUEUE.s_qw1d_run_len[st_DEC_QUEUE.w_front];
			
			// dequeue front element & increment front index.
			st_DEC_QUEUE.s_qw1d_run_len[st_DEC_QUEUE.w_front] = 0;
			if(++st_DEC_QUEUE.w_front == DEC_QUEUE_SIZE) {
				// if front reach end of queue, reset front & wake main thread.
				st_DEC_QUEUE.w_front = 0;
				pthread_mutex_unlock(&st_DEC_THREAD.thrd_mutex);
				pthread_cond_signal(&st_DEC_THREAD.thrd_cond);
			}
				
			if(is_dec_0_run) {
				// write decoded 0-run. (just increment wr_buf_bit_cnt.)
				qw_wr_buf_bit_cnt += qw_dec_run_rest_len;
				
				if(qw_wr_buf_bit_cnt>>3 >= sizeof(wr_buffer)) wr_buf_to_file(sizeof(wr_buffer));
				
			} else {
				// write decoded 1-run.
				while(qw_dec_run_rest_len > 0) {
					
					w_buf_rest_len = 8 - (qw_wr_buf_bit_cnt&0b111);
					
					if(w_buf_rest_len < qw_dec_run_rest_len) {
						// fill buffer's rest space with 1-run.
						wr_buffer[qw_wr_buf_bit_cnt>>3] |= (byte)(~0b0) >> (8 - w_buf_rest_len);
						
						qw_wr_buf_bit_cnt += w_buf_rest_len;
						qw_dec_run_rest_len -= w_buf_rest_len;
						
					} else {
						// fill buffer with rest 1-run lenghth.
						wr_buffer[qw_wr_buf_bit_cnt>>3] |= (byte)((~0b0) << (8 - qw_dec_run_rest_len)) >> (8 - w_buf_rest_len);
						
						qw_wr_buf_bit_cnt += qw_dec_run_rest_len;
						qw_dec_run_rest_len = 0;
					}
					
					if(qw_wr_buf_bit_cnt>>3 >= sizeof(wr_buffer)) wr_buf_to_file(sizeof(wr_buffer));
				}
			}
			
			is_dec_0_run = !is_dec_0_run; // toggle 0-run flag.
			
			// if queue is empty, block thread. 
			while(st_DEC_QUEUE.s_qw1d_run_len[st_DEC_QUEUE.w_front] == 0) {
				pthread_cond_signal(&st_DEC_THREAD.thrd_cond);
				pthread_cond_wait(&st_DEC_THREAD.thrd_cond, &st_DEC_THREAD.thrd_mutex);
			}
		}
		
		wr_buf_to_file(qw_wr_buf_bit_cnt+0b111 >> 3); // last write. (queue's front element value == -1)
		
		fflush(*fpp_wrb_output_file);
		
		// set queue's front element value to 0. (to indicate end of file write.)
		st_DEC_QUEUE.s_qw1d_run_len[st_DEC_QUEUE.w_front] = 0;
		pthread_mutex_unlock(&st_DEC_THREAD.thrd_mutex);
		pthread_cond_signal(&st_DEC_THREAD.thrd_cond);
		
	} while(*wp_dec_cnt < st_CONFIG.w_encoding_cnt); // repeat until all decoding is finished.
	
	return NULL;
}



qword wr_decoded_file(const qword qw_wr_bit_limit, FILE* fp_wrb_input_file, FILE* fp_wrb_output_file) {
	
	static word w_dec_cnt = 0;
	const bool use_dec_cnt = (qw_wr_bit_limit == 0);
	
	word idx;
	word w_rd_bit_len;
	Dec_Buffer uni_rd_buffer;
	const word w_uni_b_buffer_size = st_CONFIG.w_buffer_size & ~(sizeof(word)-1);
	// assign (byte array)rd_buffer to (union byte & word)uni_rd_buffer.
	byte rd_buffer[w_uni_b_buffer_size]; uni_rd_buffer.b_buffer = rd_buffer;
	
	static FILE* fp_thrd_arg_output_file;
	
	
	// define 'read encoded file and fill rd_buffer' inner function.
	word rd_file_and_fill_buf() {
		
		static const word w_w_byte_offset = (sizeof(word)>>2) + 1; // word size (in byte unit) offset.
		
		word w_rd_idx;
		word w_read_len;
		
		memset(uni_rd_buffer.b_buffer, 0x00, w_uni_b_buffer_size);
		
		if((w_read_len = fread(uni_rd_buffer.b_buffer, sizeof(byte), w_uni_b_buffer_size, fp_wrb_input_file)) > 0) {
			// if little-endian, convert union's words to big-endian.
			if(IS_LITTLE_ENDIAN) {
				for(w_rd_idx=0 ; w_rd_idx<<w_w_byte_offset<w_read_len ; w_rd_idx++)
					uni_rd_buffer.w_buffer[w_rd_idx] = byte_swap(uni_rd_buffer.w_buffer[w_rd_idx]);
			}
			
			// update progress bar.
			if(use_dec_cnt) draw_progress_bar(fp_wrb_input_file);
		}
		
		return w_read_len;
	}
	
	
	if(use_dec_cnt) {
		if(w_dec_cnt++ == 0) {
			puts("> Data decoding in progress...");
			
		} else if(w_dec_cnt > st_CONFIG.w_encoding_cnt) cleanup_resources(-1, "> Exceeded decoding count\n"); // exit(-1);
		
		// reset read file pointer & init progress bar.
		fseek(fp_wrb_input_file, 0, SEEK_SET);
		draw_progress_bar(fp_wrb_input_file);
		
		// reset write file pointer & truncate output file to 0 byte.
		fseek(fp_wrb_output_file, 0, SEEK_SET);
		ftruncate(fileno(fp_wrb_output_file), 0);
		
	} else {
		// reset write file pointer & truncate output file to 0 byte.
		fseek(fp_wrb_output_file, 0, SEEK_SET);
		ftruncate(fileno(fp_wrb_output_file), 0);
		
		// init thread-related varibales & create thread.
		pthread_mutex_init(&st_DEC_THREAD.thrd_mutex, NULL); pthread_cond_init(&st_DEC_THREAD.thrd_cond, NULL); // init mutex & cond.
		Dec_Thrd_Arg st_thrd_arg = {&w_dec_cnt, &fp_thrd_arg_output_file}; // create struct thread arguments(static variables' pointers).
		if(pthread_create(&st_DEC_THREAD.thrd_id, NULL, thrd_dequeue_and_wr_file, (void*)(&st_thrd_arg)) != 0) {
			pthread_mutex_destroy(&st_DEC_THREAD.thrd_mutex); pthread_cond_destroy(&st_DEC_THREAD.thrd_cond);
			cleanup_resources(-1, "> Failed to create thread"); // exit(-1);
		}	st_RESOURCES.thrd1d_thrd_stack[st_RESOURCES.w_thrd_stack_top++] = st_DEC_THREAD.thrd_id;
		
		// block until thread initalization is complete.
		pthread_mutex_lock(&st_DEC_THREAD.thrd_mutex);
		while(!st_DEC_THREAD.is_running) pthread_cond_wait(&st_DEC_THREAD.thrd_cond, &st_DEC_THREAD.thrd_mutex);
	}
	
	const word w_w_bit_len = (sizeof(word)<<3);
	const word w_w_bit_cnt_mask = w_w_bit_len - 1; // use to get bit_cnt in current word.
	const word w_w_bit_offset = LOG2(sizeof(word) << 2); // word size (in bit unit) offset, use to get word_idx.
	
	bool is_dec_0_run = true;
	
	word w_enc_0_cnt, w_enc_bit_cnt, w_enc_bit_rest; // (enc_0_cnt : encoded 0s in cur w_buf, enc_bit_cnt : encoded bits in cur w_buf, enc_bit_rest : remaining encoded bits to be extr.)
	qword qw_enc_0_len, qw_dec_run_len; // (qw_enc_0_len : encoded 0s total length, qw_dec_run_len : decoded 0-run/1-run length.)
	
	qword qw_total_wr_bit_cnt = 0;
	qword qw_rd_fp_ckpt_bit_pos, qw_total_wr_0_cnt; // used only '!use_dec_cnt'. (key data decoding)
	
	if(!use_dec_cnt) {
		qw_rd_fp_ckpt_bit_pos = ftell(fp_wrb_input_file) << 3;
		qw_total_wr_0_cnt = 0;
	}
	
	word w_rd_buf_bit_cnt = 0; // (rd_bit_cnt >> bit_offset) : rd_w_buf_idx, (rd_bit_cnt & mask) : bit_cnt in current rd_w_buf.
	w_rd_bit_len = rd_file_and_fill_buf() << 3; // fill rd_buffer & store read_len in bit units.
	
	// pass output file's fp to thread & reset queue.
	fp_thrd_arg_output_file = fp_wrb_output_file;
	memset(&st_DEC_QUEUE, 0x00, sizeof(st_DEC_QUEUE));
	
	// decode input file & enqueue decoded 0-run/1-run length to write output file.
	while((w_rd_bit_len > 0) && (use_dec_cnt || qw_total_wr_bit_cnt<qw_wr_bit_limit)) {
		
		// count encoded code's prefix length.
		for(qw_enc_0_len=0 ; w_rd_bit_len > 0 ; ) {
			if(uni_rd_buffer.w_buffer[w_rd_buf_bit_cnt>>w_w_bit_offset] == 0) {
				// if cur w_buf value == 0x00, increase w_enc_0_len by unread bits in cur w_buf.
				w_enc_0_cnt = w_w_bit_len - (w_rd_buf_bit_cnt & w_w_bit_cnt_mask);
				qw_enc_0_len += w_enc_0_cnt;
				
			} else {
				// if cur w_buf value != 0x00, count 0s from MSB until first 0b1 & << count of 0s.
				if((w_enc_0_cnt = cnt_leading_zero(uni_rd_buffer.w_buffer[w_rd_buf_bit_cnt>>w_w_bit_offset])) != 0) {
					uni_rd_buffer.w_buffer[w_rd_buf_bit_cnt>>w_w_bit_offset] <<= w_enc_0_cnt;
					qw_enc_0_len += w_enc_0_cnt;
					
				} else break; // if cur w_buf MSB == 0b1, break.
			}
			
			// (if w_rd_buf_bit_cnt > w_rd_bit_len, 0s are padding, not encoded code.)
			if((w_rd_buf_bit_cnt += w_enc_0_cnt) >= w_rd_bit_len) {
				if(!use_dec_cnt) qw_rd_fp_ckpt_bit_pos += w_rd_bit_len; // if !use_dec_cnt(key data), update fp pos in bit units.
				w_rd_buf_bit_cnt = 0;
				w_rd_bit_len = rd_file_and_fill_buf() << 3;
			}
		}
		
		// decode encoded 0-run/1-run code.
		if(w_rd_bit_len > 0) {
			
			if(is_dec_0_run) {
				// if prefix length == 0, exreact 2bits (0-run len == 1 or 2); otherwise, extract 'prefix length + 1'bits (0-run len > 2).
				w_enc_bit_rest = (qw_enc_0_len>0 ? qw_enc_0_len+1 : 2);
				
				// extract encoded 0-run code's suffix bit.
				for(qw_dec_run_len=0b0 ; ; ) {
					// set w_enc_bit_cnt. ((w_w_bit_len-(...)) == read buffer's rest space length.)
					if(w_enc_bit_rest > (w_w_bit_len-(w_rd_buf_bit_cnt&w_w_bit_cnt_mask))) {
						w_enc_bit_cnt = (w_w_bit_len-(w_rd_buf_bit_cnt&w_w_bit_cnt_mask));
						
					} else w_enc_bit_cnt = w_enc_bit_rest;
					
					// insert extracted encoded bits into qw_dec_run_len from LSB.
					qw_dec_run_len <<= w_enc_bit_cnt;
					qw_dec_run_len |= uni_rd_buffer.w_buffer[w_rd_buf_bit_cnt>>w_w_bit_offset] >> (w_w_bit_len - w_enc_bit_cnt);
					
					w_enc_bit_rest -= w_enc_bit_cnt; // decrement rest encoded bits by extreacted amount.
					uni_rd_buffer.w_buffer[w_rd_buf_bit_cnt>>w_w_bit_offset] <<= w_enc_bit_cnt;
					
					if((w_rd_buf_bit_cnt += w_enc_bit_cnt) == w_rd_bit_len) {
						if(!use_dec_cnt) qw_rd_fp_ckpt_bit_pos += w_rd_bit_len;
						w_rd_buf_bit_cnt = 0;
						w_rd_bit_len = rd_file_and_fill_buf() << 3;
						
					} else if(w_rd_buf_bit_cnt > w_rd_bit_len) cleanup_resources(-1, "> Read buffer overflow has occurred. rd_bit_len: %zu, rd_buf_bit_cnt: %zu\n", w_rd_bit_len, w_rd_buf_bit_cnt); // exit(-1);
					
					if(w_enc_bit_rest > 0) {
						// check for input data corruption.
						if(w_rd_bit_len == 0) cleanup_resources(-1, "> Corrupted input data: Incomplete encoded code\n"); // exit(-1);
						
					} else break; // if all encoded bits are extracted, break.
				}
				
				// calc decoded 0-run length from extracted encoded bits.
				qw_dec_run_len += (qw_enc_0_len>0 ? 1 : -1);
				
			} else {
				// calc decoded 1-run length from encoded 1-run's prefix length.
				qw_dec_run_len = qw_enc_0_len + 1;
				uni_rd_buffer.w_buffer[w_rd_buf_bit_cnt>>w_w_bit_offset] <<= 1;
			
				if((w_rd_buf_bit_cnt += 1) == w_rd_bit_len) {
					if(!use_dec_cnt) qw_rd_fp_ckpt_bit_pos += w_rd_bit_len;
					w_rd_buf_bit_cnt = 0;
					w_rd_bit_len = rd_file_and_fill_buf() << 3;
				}
			}
			
			// if queue is full, block.
			while(st_DEC_QUEUE.s_qw1d_run_len[st_DEC_QUEUE.w_rear] > 0) {
				pthread_cond_signal(&st_DEC_THREAD.thrd_cond);
				pthread_cond_wait(&st_DEC_THREAD.thrd_cond, &st_DEC_THREAD.thrd_mutex);
			}
			
			// enqueue rear element & increment rear index.
			st_DEC_QUEUE.s_qw1d_run_len[st_DEC_QUEUE.w_rear] = qw_dec_run_len; // store decoded run length in queue's rear element.
			if(++st_DEC_QUEUE.w_rear == DEC_QUEUE_SIZE) {
				// if rear reach end of queue, reset rear & wake thread.
				st_DEC_QUEUE.w_rear = 0;
				pthread_mutex_unlock(&st_DEC_THREAD.thrd_mutex);
				pthread_cond_signal(&st_DEC_THREAD.thrd_cond);
			}	
			
			// update return values.
			qw_total_wr_bit_cnt += qw_dec_run_len;
			if(!use_dec_cnt && is_dec_0_run) qw_total_wr_0_cnt += qw_dec_run_len;
			
			is_dec_0_run = !is_dec_0_run;
		}
	}
	
	// check for input data corruption.
	if((qw_total_wr_bit_cnt&0b111) > 0) cleanup_resources(-1, "> Corrupted input data: Residual bit error after decoding.\n" "resid_wr_bit_cnt: %llu\n", qw_total_wr_bit_cnt&0b111); // exit(-1);
	
	if(!use_dec_cnt) {
		// check for key data corruption.
		if(qw_total_wr_bit_cnt != qw_wr_bit_limit) cleanup_resources(-1, "> Corrupted key data: Record bits length and decoded bit count mismatch.\n" "rec_bits_len: %llu, dec_bit_cnt: %llu\n", qw_wr_bit_limit, qw_total_wr_bit_cnt); // exit(-1);
		
		// set read file pointer to updated fp pos & update progress bar.
		qw_rd_fp_ckpt_bit_pos += w_rd_buf_bit_cnt;
		fseeko(fp_wrb_input_file, qw_rd_fp_ckpt_bit_pos+0b111 >> 3, SEEK_SET);
		draw_progress_bar(fp_wrb_input_file);
	}
	
	// if queue is full, block until queue is empty, and then send enqueue end signal(='-1') to thread.
	while(st_DEC_QUEUE.s_qw1d_run_len[st_DEC_QUEUE.w_rear] > 0) pthread_cond_wait(&st_DEC_THREAD.thrd_cond, &st_DEC_THREAD.thrd_mutex);
	st_DEC_QUEUE.s_qw1d_run_len[st_DEC_QUEUE.w_rear] = -1;
	pthread_mutex_unlock(&st_DEC_THREAD.thrd_mutex);
	pthread_cond_signal(&st_DEC_THREAD.thrd_cond);
	
	if(w_dec_cnt < st_CONFIG.w_encoding_cnt) {
		// block until thread finishes dequeue.
		while(st_DEC_QUEUE.s_qw1d_run_len[st_DEC_QUEUE.w_rear] < 0) pthread_cond_wait(&st_DEC_THREAD.thrd_cond, &st_DEC_THREAD.thrd_mutex);
		
	} else cleanup_thread(); // if last decoding, clean up thread.
	
	return use_dec_cnt ? qw_total_wr_bit_cnt>>3 : qw_total_wr_0_cnt;
}



static inline void cleanup_thread() {
	
	pthread_mutex_unlock(&st_DEC_THREAD.thrd_mutex);
	pthread_cond_signal(&st_DEC_THREAD.thrd_cond);
	
	// wait for thread termination.
	pthread_join(st_RESOURCES.thrd1d_thrd_stack[--st_RESOURCES.w_thrd_stack_top], NULL);
	
	pthread_mutex_destroy(&st_DEC_THREAD.thrd_mutex);
	pthread_cond_destroy(&st_DEC_THREAD.thrd_cond);
	
	return;
}
