#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <alloca.h>

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

static unsigned bytesPerSymbol()
{
	assert(DICTSIZE <= (1llu << (CHAR_BIT * sizeof(unsigned))));
	unsigned symBytes = 0;
	unsigned dictSize = DICTSIZE - 1;
	while (dictSize)
	{
		symBytes++;
		dictSize = dictSize >> CHAR_BIT;
	}
	return symBytes;
}

static void pack(unsigned *input, size_t nmemb, uint8_t *output)
{
	unsigned symBytes = bytesPerSymbol();
	for (size_t i = 0; i < nmemb; i++)
	{
		for (unsigned b = 0; b < symBytes; b++)
		{
			output[i * symBytes + b] = (uint8_t)
				((input[i] >> (CHAR_BIT * b)) & 0xff);
		}
	}
}

static void unpack(uint8_t *input, size_t nmemb, unsigned *output)
{
	unsigned symBytes = bytesPerSymbol();
	size_t symCount = nmemb / symBytes;

	for (size_t i = 0; i < symCount; i++)
	{
		unsigned value = 0;
		for (unsigned b = 0; b < symBytes; b++)
		{
			value += input[i * symBytes + b] << (b * CHAR_BIT);
		}

		output[i] = value;
	}
}

static int encode(void)
{
	struct lzw_state enc;
	lzw_state_init(&enc);

	char input[1000];
	unsigned compressed[1024];
	unsigned symBytes = bytesPerSymbol();
	uint8_t *writeBuffer = alloca(1024 * symBytes);
	size_t len;
	unsigned *comp_curs;
	unsigned complen;

	while ((len = fread(input, sizeof(char), 1000, stdin)) > 0) {
		comp_curs = &compressed[0];
		complen = 0;
		for(unsigned i = 0; i < len; i++)
			assert(!lzw_encode(&enc, emit_code, (void *)&comp_curs, input[i]));
		complen = comp_curs - compressed;

		pack(compressed, complen, writeBuffer);
		if (fwrite(writeBuffer, symBytes, complen, stdout) != complen)
			return -1;
	}

	comp_curs = compressed;
	assert(!lzw_encode_finish(&enc, emit_code, (void *) &comp_curs));
	complen = comp_curs - compressed;
	pack(compressed, complen, writeBuffer);
	if (fwrite(writeBuffer, symBytes, complen, stdout) != complen)
		return -1;

	return 0;
}

static int decode(void)
{
	struct lzw_state dec;
	lzw_state_init(&dec);

	unsigned symBytes = bytesPerSymbol();
	uint8_t *readBuffer = alloca(50 * symBytes);
	unsigned input[50];
	unsigned char output[1024];
	size_t len;

	while ((len = fread(readBuffer, symBytes, 50, stdin)) > 0) {
		unpack(readBuffer, len * symBytes, input);
		unsigned char *dec_curs = &output[0];
		for(unsigned i = 0; i < len; i++)
			assert(!lzw_decode(&dec, emit_char, (void *)&dec_curs, input[i]));
		unsigned declen = dec_curs - output;

		if (fwrite(output, sizeof(char), declen, stdout) != declen)
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
