default: lzw
lzw: lzw.c
	gcc -Wall -Wextra -g3 --std=c99 -o $@ $^
clean:
	rm lzw
