default: lzw_test
lzw_test: lzw.c lzw_test.c
	gcc -Wall -Wextra -g3 --std=c99 -DDICTSIZE=260 -o $@ $^
clean:
	rm lzw
