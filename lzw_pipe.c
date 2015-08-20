#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <alloca.h>

#include "lzw.h"

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
			comp_curs += lzw_encode(&enc, input[i], comp_curs);
		complen = comp_curs - compressed;

		pack(compressed, complen, writeBuffer);
		if (fwrite(writeBuffer, symBytes, complen, stdout) != complen)
			return -1;
	}

	comp_curs = compressed;
	comp_curs += lzw_encode_finish(&enc, comp_curs);
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

	unsigned char tempbuf[DICTSIZE];
	unsigned char *tempbuf_end = tempbuf + DICTSIZE;
	while ((len = fread(readBuffer, symBytes, 50, stdin)) > 0) {
		unpack(readBuffer, len * symBytes, input);
		unsigned char *dec_curs = &output[0];
		for(unsigned i = 0; i < len; i++) {
			unsigned char *tempbuf_curs = lzw_decode(&dec, input[i], tempbuf);
			memcpy(dec_curs, tempbuf_curs, (tempbuf_end - tempbuf_curs));
			dec_curs += (tempbuf_end - tempbuf_curs);
		}
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
