#include <stdio.h>
#include <assert.h>

#include "lzw.h"

int emit_code(void *p, unsigned code)
{
	//printf("encode -> %u\n", code);
	**(unsigned **)p = code;
	*(unsigned**)p = *(unsigned**)p + 1;
	return 1;
}

int emit_char(void *p, unsigned char ch)
{
	//printf("decode -> %c\n", ch);
	**(unsigned char**)p = ch;
	*(unsigned char**)p = *(unsigned char**)p + 1;
	return 1;
}

char *tests[] = {
	"a",
	"aa",
	"aaa",
	"aaaa",
	"aaaaa",
	"aaaaaa",
	"aba",
	"abaa",
	"abaaa",
	"abaaaa",
	"abcaaaa",
	"abcdaaaa",
	"abcdeaaaa",
	"abcdefaaaa",
	"abcdefgaaaa",
	"abcdefghaaaa",
	"ababcbababaaaaaa",
	"abcabcabcabcabcabcabcabcabcabcabcabc",
	NULL,
};

int main(void)
{
	for(unsigned n = 0; tests[n]; n++) {
		char *input = tests[n];
		unsigned compressed[1024];
		unsigned char output[1024];
	
		struct lzw_state enc;
		lzw_state_init(&enc);
	
		unsigned *comp_curs = &compressed[0];
		unsigned complen = 0;
		for(unsigned i = 0; input[i]; i++)
			complen += lzw_encode(&enc, emit_code, (void *)&comp_curs, input[i]);
		complen += lzw_encode_finish(&enc, emit_code, (void *)&comp_curs);
	
	
		struct lzw_state dec;
		lzw_state_init(&dec);
	
		unsigned char *dec_curs = &output[0];
		unsigned declen = 0;
		for(unsigned i = 0; i < complen; i++)
			declen += lzw_decode(&dec, emit_char, (void *)&dec_curs, compressed[i]);
	
		for(unsigned i = 0; input[i]; i++)
			assert(input[i] == output[i]);
	}

	return 0;
}
