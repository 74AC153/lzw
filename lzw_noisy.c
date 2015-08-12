#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "lzw.h"

void lzw_state_init(struct lzw_state *state)
{
	for(unsigned i = 0; i < SYMCOUNT; i++) {
		state->dict[i].code = i;
		state->dict[i].parent = NULL;
		state->dict[i].sibling = NULL;
		state->dict[i].child = NULL;
		state->dict[i].ch = i;
	}
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
	for(unsigned i = 0; i < SYMCOUNT; i++) {
		state->dict[i].child = NULL;
	}
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
	assert(state->next_code < 4095);
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
	emit_code_fn_t emit,
	void *p,
	unsigned char ch)
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
		status = emit(p, state->current->code);
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

int lzw_encode_finish(
	struct lzw_state *state,
	emit_code_fn_t emit,
	void *p)
{
	printf("encode (finish) emit %u\n", state->current->code);
	return emit(p, state->current->code);
}

static unsigned char first(struct dict_entry *entry)
{
	if(entry->parent == NULL)
		return entry->ch;
	else return first(entry->parent);
}

typedef int (*emit_char_fn_t)(void *p, unsigned char ch);
int lzw_decode(
	struct lzw_state *state,
	emit_char_fn_t emit,
	void *p,
	unsigned code)
{
	void output(struct dict_entry *ent)
	{
		if(ent->parent)
			output(ent->parent);
		printf("decode emit %c\n", ent->ch);
		emit(p, ent->ch);
	}

	int status = 0;
	struct dict_entry *entry, *child;
	printf("decode %u\n", code);
	if(state->current == NULL) {
		printf("decode emit %c\n", state->dict[code].ch);
		status = emit(p, state->dict[code].ch);
		state->current = state->dict + state->dict[code].ch;
	} else if(code >= state->next_code) {
		output(state->current);
		emit(p, first(state->current));

		child = alloc_dict_entry(state, state->current, first(state->current));
		assert(child); // encoder didn't reset, so we better not
		printf("decode add dict %u ", child->code);
		print_reverse(child);
		printf("\n");

		state->current = step(state->current, first(state->current));
	} else {
		entry = state->dict + code;
		output(state->dict + code);

		child = alloc_dict_entry(state, state->current, first(state->dict + code));
		if(!child) {
			printf("decode reset dict\n");
			reset_dict(state);
		} else {
			printf("decode add dict %u ", child->code);
			print_reverse(child);
			printf("\n");
		}

		state->current = entry;
	}

	printf("decode cursor @ ");
	print_reverse(state->current);
	printf(" (%u)\n", state->current->code);

	return status;
}
