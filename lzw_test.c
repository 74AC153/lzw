#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "lzw.h"

int emit_code(void *p, unsigned code)
{
	//printf("encode -> %u\n", code);
	**(unsigned **)p = code;
	*(unsigned**)p = *(unsigned**)p + 1;
	return 0;
}

int emit_char(void *p, unsigned char ch)
{
	//printf("decode -> %c\n", ch);
	**(unsigned char**)p = ch;
	*(unsigned char**)p = *(unsigned char**)p + 1;
	return 0;
}

char *tests[] = {
	"a",
	"aa",
	"aaa",
	"aaaa",
	"aaaaa",
	"aaaaaa",
	"aba",
	"aaab",
	"aaa\n",
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

// for debugging
static void print_reverse(struct dict_entry *entry)
{
	if(entry->parent)
		print_reverse(entry->parent);
	printf("%c", entry->ch);
}
void print_dict_item(struct lzw_state *state, unsigned code)
{
	print_reverse(state->dict + code);
}
void print_dict(struct lzw_state *state)
{
	for(unsigned i = 256; i < state->next_code; i++) {
		printf("%u ", i);
		print_dict_item(state, i);
		printf("\n");
	}
}

int main(void)
{
	unsigned char outbuf[DICTSIZE];
	unsigned char *outbuf_end = outbuf + DICTSIZE;
	struct lzw_state enc;
	struct lzw_state dec;

	for(unsigned n = 0; tests[n]; n++) {
		char *input = tests[n];
		unsigned compressed[1024];
		unsigned char output[1024];

		printf(" input: %s\n", input);
	
		lzw_state_init(&enc);
	
		unsigned *comp_curs = &compressed[0];
		unsigned complen = 0;
		for(unsigned i = 0; input[i]; i++)
			comp_curs += lzw_encode(&enc, input[i], comp_curs);
		comp_curs += lzw_encode_finish(&enc, comp_curs);
		complen = comp_curs - compressed;
		printf("compressed: ");
		for(unsigned i = 0; i < complen; i++) {
			printf("%u (", compressed[i]);
			print_dict_item(&enc, compressed[i]);
			printf(") ");
		}
		printf("\n");
	
		memset(output, 0, sizeof(output));
		lzw_state_init(&dec);
	
		unsigned char *dec_curs = &output[0];
		for(unsigned i = 0; i < complen; i++) {
			unsigned char *outbuf_curs = lzw_decode(&dec, compressed[i], outbuf);
			memcpy(dec_curs, outbuf_curs, (outbuf_end - outbuf_curs));
			dec_curs += (outbuf_end - outbuf_curs);
		}
		unsigned declen = dec_curs - output;
		printf("output: %*s (%u)\n\n", declen, output, declen);
	
		for(unsigned i = 0; input[i]; i++)
			assert(input[i] == output[i]);
		assert(declen == strlen(input));
	}

	return 0;
}
