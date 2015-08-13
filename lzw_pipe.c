#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "lzw.h"

static int emit_code(void *p, unsigned code)
{
	**(unsigned **)p = code;
	*(unsigned**)p = *(unsigned**)p + 1;
	return 0;
}

static int emit_char(void *p, unsigned char ch)
{
	**(unsigned char**)p = ch;
	*(unsigned char**)p = *(unsigned char**)p + 1;
	return 0;
}

static int encode(void)
{
	struct lzw_state enc;
	lzw_state_init(&enc);

	char input[1000];
	unsigned compressed[1024];
	size_t len;
	unsigned *comp_curs;
	unsigned complen;

	while ((len = fread(input, 1, 1000, stdin)) > 0) {
		comp_curs = &compressed[0];
		complen = 0;
		for(unsigned i = 0; i < len; i++)
			assert(!lzw_encode(&enc, emit_code, (void *)&comp_curs, input[i]));
		complen = comp_curs - compressed;

		if (fwrite(compressed, sizeof(unsigned), complen, stdout) != complen)
			return -1;
	}

	comp_curs = compressed;
	assert(!lzw_encode_finish(&enc, emit_code, (void *) &comp_curs));
	complen = comp_curs - compressed;
	if (fwrite(compressed, sizeof(unsigned), complen, stdout) != complen)
		return -1;

	return 0;
}

static int decode(void)
{
	struct lzw_state dec;
	lzw_state_init(&dec);

	unsigned input[50];
	unsigned char output[1024];
	size_t len;

	while ((len = fread(input, sizeof(unsigned), 50, stdin)) > 0) {
		unsigned char *dec_curs = &output[0];
		for(unsigned i = 0; i < len; i++)
			assert(!lzw_decode(&dec, emit_char, (void *)&dec_curs, input[i]));
		unsigned declen = dec_curs - output;

		if (fwrite(output, sizeof(unsigned), declen, stdout) != declen)
			return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 1)
		return encode();
	else if (argc == 2 && strcmp("-d", argv[1]) == 0)
		return decode();

	fprintf(stderr, "Usage:\n\tlzw_pipe [-d]\n");
	return -1;
}
