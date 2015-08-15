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

// all (should) return 0 on success, -1 on error
typedef int (*emit_code_fn_t)(void *p, unsigned code);
int lzw_encode(
	struct lzw_state *state,
	emit_code_fn_t emit,
	void *p,
	unsigned char ch);

int lzw_encode_finish(
	struct lzw_state *state,
	emit_code_fn_t emit,
	void *p);

typedef int (*emit_char_fn_t)(void *p, unsigned char ch);
int lzw_decode(
	struct lzw_state *state,
	emit_char_fn_t emit,
	void *p,
	unsigned code);

#endif
