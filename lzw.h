#if ! defined(LZW_H_INCLUDED)
#define LZW_H_INCLUDED
struct dict_entry {
	unsigned code;
	struct dict_entry *parent;
	struct dict_entry *sibling;
	struct dict_entry *child;
	unsigned char ch;
};

#if ! defined(DICTSIZE)
#define DICTSIZE 4096
#endif

#if ! defined(SYMCOUNT)
#define SYMCOUNT 256 // other values e.g., 2 (binary) 128 (7-bit ascii)
#endif

struct lzw_state {
	struct dict_entry *current;
	unsigned next_code;
	struct dict_entry dict[DICTSIZE];
};

void lzw_state_init(struct lzw_state *state);

// returns:
// 1: *code output
// 0: no output yet
int lzw_encode(
	struct lzw_state *state,
	unsigned char ch,
	unsigned *code);

// always returns 1
int lzw_encode_finish(
	struct lzw_state *state,
	unsigned *code);

// returns pointer into outbuf at start of emitted chars.
// chars from return value to &outbuf[DICTSIZE-1] contain output
unsigned char *lzw_decode(
	struct lzw_state *state,
	unsigned code,
	unsigned char outbuf[DICTSIZE]);

#endif
