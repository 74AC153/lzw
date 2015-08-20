#include <stdlib.h>
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

int lzw_encode(
	struct lzw_state *state,
	unsigned char ch,
	unsigned *code)
{
	int status = 0;
	struct dict_entry *child;
	if(state->current == NULL) {
		state->current = state->dict + ch;
	} else if((child = step(state->current, ch))) {
		// have transition via ch
		state->current = child;
	} else {
		// don't have transition via ch
		*code = state->current->code;
		status = 1;
		child = alloc_dict_entry(state, state->current, ch);
		if(!child)
			reset_dict(state);
		
		state->current = state->dict + ch;
	}

	return status;
}

int lzw_encode_finish(struct lzw_state *state, unsigned *code)
{
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

		// we also need add a dict entry with the current state chain's first
		// entry appended, and step using that character
		child = alloc_dict_entry(state, state->current, entry->ch);
		assert(child); // encoder didn't reset, so we better not
		state->current = step(state->current, entry->ch);
	} else {
		for(entry = state->dict + code; entry->parent; entry = entry->parent) 
			*(--outbuf_curs) = entry->ch;
		// entry points to first item in state chain here
		*(--outbuf_curs) = entry->ch;

		child = alloc_dict_entry(state, state->current, entry->ch);
		if(!child)
			reset_dict(state);

		state->current = state->dict + code;
	}

	return outbuf_curs;
}
