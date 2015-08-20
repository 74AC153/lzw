default: lzw_test lzw_test_noisy lzw_pipe lzw_pipe2
lzw_test: lzw.c lzw_test.c
	gcc -Wall -Wextra -g3 --std=c99 -o $@ $^
lzw_test_noisy: lzw_noisy.c lzw_test.c
	gcc -Wall -Wextra -g3 --std=c99 -o $@ $^
lzw_pipe: lzw.c lzw_pipe.c
	gcc -Wall -Wextra -g3 --std=c99 -DDICTSIZE=65536 -o $@ $^
lzw_pipe2: lzw.c lzw_pipe2.c
	gcc -Wall -Wextra -g3 --std=c99 -D_XOPEN_SOURCE=2 -o $@ $^
clean:
	-rm lzw_test lzw_test_noisy lzw_pipe lzw_pipe2
