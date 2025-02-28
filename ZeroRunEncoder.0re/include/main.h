#ifndef MAIN_H
#define MAIN_H

#include "common.h"

#include "keymaker.h"
#include "converter.h"
#include "encoder.h"
#include "decoder.h"
#include "utils.h"

#ifdef _WIN32
	#define ftello _ftelli64
#endif

#define NAME_MAX 255
#define MAX_DUP_NAME 9999

// generate unique file name.
static inline void gen_uniq_file_name(char* cp_file_path);
// write 0re file's signature, struct key data, encoded file data to 0re file.
static inline void wr_0re_file(Key* stp_key, FILE* fp_wrb_encoded_file, FILE* fp_wb_0re_file);
// read 0re file's signature, struct key data, encoded file data from 0re file.
static inline void rd_0re_file(Key* stp_key, FILE* fp_rb_0re_file, FILE* fp1d_wrb_key_file[2], FILE* fp_wrb_temp_file);
// signal handler function : handles process cleanup on signal interruption, segmentation fault, termination.
static void sig_cleanup_handler(int sig);
	
#endif
