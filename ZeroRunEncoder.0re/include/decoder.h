#ifndef DECODER_H
#define DECODER_H

#include "common.h"

#ifdef _WIN32
	#define fseeko _fseeki64
#endif

#if __SIZEOF_POINTER__ == 8
	#define cnt_leading_zero __builtin_clzll
	#define byte_swap __builtin_bswap64
#else
	#define cnt_leading_zero __builtin_clz
	#define byte_swap __builtin_bswap32
#endif

#define IS_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

#define DEC_QUEUE_SIZE __SIZEOF_POINTER__<<10
#define LOG2(x) (sizeof(word)<<3) - cnt_leading_zero(x)

typedef struct Dec_Queue {
	word w_front;
	s_qword s_qw1d_run_len[DEC_QUEUE_SIZE];
	word w_rear;
} Dec_Queue;

typedef struct Dec_Thread {
	pthread_t thrd_id;
	pthread_mutex_t thrd_mutex;
	pthread_cond_t thrd_cond;
	bool is_running;
} Dec_Thread;

typedef struct Dec_Thrd_Arg {
	word* wp_dec_cnt;
	FILE** fpp_thrd_arg_output_file;
} Dec_Thrd_Arg; // for pass arguments to thread.

typedef union Dec_Buffer {
	byte* b_buffer;
	word* w_buffer;
} Dec_Buffer;

// thread function : dequeue and write decoded codes to output file.
void* thrd_dequeue_and_wr_file();
// decode encoded codes and write to output file using thread function. (if wr_bit_limit > 0, decode key data and return chunk's map length; otherwise, return length of decoded bits in bytes.)
qword wr_decoded_file(const qword qw_wr_bit_limit, FILE* fp_wrb_input_file, FILE* fp_wrb_output_file);
// clean up thread.
static inline void cleanup_thread();

#endif
