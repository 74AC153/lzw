#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "lzw.h"

void lzw_state_init(struct lzw_state *state)
{
	for(unsigned i = 0; i < SYMCOUNT; i++)
		state->dict[i] = (struct dict_entry){ i, NULL, NULL, NULL, i };
	state->next_code = SYMCOUNT;
	state->current = NULL;
}

static struct dict_entry *step(struct dict_entry *parent, unsigned char ch)
{
	struct dict_entry *child;
	for(child = parent->child; child; child = child->sibling)
		if(child->ch == ch)
			break;
	return child;
}

static void reset_dict(struct lzw_state *state)
{
	state->next_code = SYMCOUNT;
	for(unsigned i = 0; i < SYMCOUNT; i++)
		state->dict[i].child = NULL;
}

static struct dict_entry *alloc_dict_entry(
	struct lzw_state *state,
	struct dict_entry *parent,
	unsigned char ch)
{
	if(state->next_code == DICTSIZE)
		return NULL;
	struct dict_entry *entry = state->dict + state->next_code;
	entry->code = state->next_code++;
	assert(state->next_code <= DICTSIZE);
	entry->ch = ch;

	entry->sibling = parent->child;
	parent->child = entry;
	entry->parent = parent;
	entry->child = NULL;
	return entry;
}

static void print_reverse(struct dict_entry *entry)
{
	if(entry->parent)
		print_reverse(entry->parent);
	printf("%c", entry->ch);
}

typedef int (*emit_code_fn_t)(void *p, unsigned code);
int lzw_encode(
	struct lzw_state *state,
	unsigned char ch,
	unsigned *code)
{
	int status = 0;
	struct dict_entry *child;
	printf("encode '%c'\n", ch);
	if(state->current == NULL) {
		printf("encode ... (!current)\n");
		state->current = state->dict + ch;
	} else if((child = step(state->current, ch))) {
		printf("encode ... (step)\n");
		// have transition via ch
		state->current = child;
	} else {
		printf("encode emit %u\n", state->current->code);
		// don't have transition via ch
		*code = state->current->code;
		status = 1;
		child = alloc_dict_entry(state, state->current, ch);
		if(!child) {
			printf("encode reset dict\n");
			reset_dict(state);
		} else {
			printf("encode add dict %u ", child->code);
			print_reverse(child);
			printf("\n");
		}
		
		state->current = state->dict + ch;
	}
	printf("encode cursor @ ");
	print_reverse(state->current);
	printf(" (%u)\n", state->current->code);

	return status;
}

int lzw_encode_finish(struct lzw_state *state, unsigned *code)
{
	printf("encode (finish) emit %u\n", state->current->code);
	*code = state->current->code;
	return 1;
}

unsigned char *lzw_decode(
	struct lzw_state *state,
	unsigned code,
	unsigned char outbuf[DICTSIZE])
{
	unsigned char *outbuf_curs = &outbuf[DICTSIZE];
	struct dict_entry *entry, *child;

	if(state->current == NULL) {
		*(--outbuf_curs) = state->dict[code].ch; 

		state->current = state->dict + state->dict[code].ch;
	} else if(code >= state->next_code) {
		// this is a bit tricky: we need to emit the first character in the
		// state chain leading up to current both first and last in the output.
		unsigned char *last = --outbuf_curs;
		for(entry = state->current ; entry->parent; entry = entry->parent) 
			*(--outbuf_curs) = entry->ch;
		// entry points to first item in state chain here
		*(--outbuf_curs) = *last = entry->ch;
		for(unsigned char *curs = outbuf_curs+1; curs < outbuf+DICTSIZE; curs++)
			printf("decode emit %c ", *curs);

		// we also need add a dict entry with the current state chain's first
		// entry appended, and step using that character
		child = alloc_dict_entry(state, state->current, entry->ch);
		assert(child); // encoder didn't reset, so we better not
		printf("decode add dict %u ", child->code);
		print_reverse(child);
		printf("\n");

		state->current = step(state->current, entry->ch);
	} else {
		for(entry = state->dict + code; entry->parent; entry = entry->parent) 
			*(--outbuf_curs) = entry->ch;
		// entry points to first item in state chain here
		*(--outbuf_curs) = entry->ch;
		for(unsigned char *curs = outbuf_curs+1; curs < outbuf+DICTSIZE; curs++)
			printf("decode emit %c\n", *curs);

		child = alloc_dict_entry(state, state->current, entry->ch);
		if(!child) {
			printf("decode reset dict\n");
			reset_dict(state);
		} else {
			printf("decode add dict %u ", child->code);
			print_reverse(child);
			printf("\n");
		}

		state->current = state->dict + code;
	}

	printf("decode cursor @ ");
	print_reverse(state->current);
	printf(" (%u)\n", state->current->code);

	return outbuf_curs;
}
