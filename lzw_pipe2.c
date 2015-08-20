#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "lzw.h"

#if DICTSIZE != 4096
#error this program must be compiled with DICTSIZE == 4096
#endif

void out_queue(unsigned code, FILE *outfile, _Bool finish)
{
	assert(code < 4096);
fprintf(stderr, "out %u\n", code);
	static _Bool have_hold = 0;
	static unsigned hold;
	if(have_hold) {
		unsigned char outbuf[3];
		outbuf[0] = hold & 0xFF;
		outbuf[1] = ((hold >> 8) & 0xF) | ((code << 4) & 0xF0);
		outbuf[2] = (code >> 4) & 0xFF;
		fprintf(stderr, "write %u %u\n", hold, code);
		fwrite(outbuf, 1, 3, outfile);
		have_hold = 0;
	} else if(finish) {
		unsigned char outbuf[2];
		outbuf[0] = code & 0xFF;
		outbuf[1] = ((code >> 8) & 0xF);
		fprintf(stderr, "write %u\n", code);
		fwrite(outbuf, 1, 2, outfile);
	} else {
		hold = code;
		have_hold = 1;
		fprintf(stderr, "hold %u\n", hold);
	}
}

int in_dequeue(unsigned *code, FILE *infile)
{
	static _Bool have_hold = 0;
	static unsigned hold;
	if(have_hold) {
		*code = hold;
fprintf(stderr, "in %u\n", *code);
		have_hold = 0;
		return 1;
	} else {
		unsigned char inbuf[3] = {0, 0, 0};
		int n = fread(inbuf, 1, 3, infile);
		if(n <= 0) {
			if(n < 0)
				perror("fread()");
			return 0;
		}
		*code = inbuf[0];
		*code |= (inbuf[1] & 0xF) << 8;
fprintf(stderr, "in %u\n", *code);
		if(n == 3) {
			hold = inbuf[1] >> 4;
			hold |= inbuf[2] << 4;
			have_hold = 1;
		}
		return 1;
	}
}

int main(int argc, char *argv[])
{
	int opt;
	FILE *infile = stdin;
	FILE *outfile = stdout;
	_Bool decomp = 0;
	while((opt = getopt(argc, argv, "i:o:dh")) != -1) {
		switch(opt) {
		case 'i':
			infile = fopen(optarg, "rb");
			if(!infile) {
				perror("fopen(infile)");
				return -1;
			}
			break;
		case 'o':
			outfile = fopen(optarg, "wb");
			if(!outfile) {
				perror("fopen(outfile)");
				return -1;
			}
			break;
		case 'd':
			decomp = 1;
			break;
		case 'h':
			fprintf(stderr, "usage: %s [-i <infile>] [-o <outfile>] [-d] [-h]\n",
			        argv[0]);
			return -1;
		}
	}

	struct lzw_state lzw;
	lzw_state_init(&lzw);

	if(! decomp) {
		int ch_in;
		unsigned code_out;
		while((ch_in = fgetc(infile)) != EOF)
			if(lzw_encode(&lzw, ch_in, &code_out))
				out_queue(code_out, outfile, 0);
		lzw_encode_finish(&lzw, &code_out);
		out_queue(code_out, outfile, 1);
	} else {
		unsigned code_in;
		unsigned char outbuf[DICTSIZE], *outbuf_end = outbuf + DICTSIZE;
		while(in_dequeue(&code_in, infile)) {
			unsigned char *outbuf_curs = lzw_decode(&lzw, code_in, outbuf);
			fwrite(outbuf_curs, outbuf_end - outbuf_curs, 1, outfile);
		}
	}
	return 0;
}
