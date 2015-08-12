default: lzw_test lzw_test_noisy
lzw_test: lzw.c lzw_test.c
	gcc -Wall -Wextra -g3 --std=c99 -DDICTSIZE=260 -o $@ $^
lzw_test_noisy: lzw_noisy.c lzw_test.c
	gcc -Wall -Wextra -g3 --std=c99 -DDICTSIZE=260 -o $@ $^
clean:
	rm lzw_test lzw_test_noisy
