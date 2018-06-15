make:	main.c
	gcc -g2 -gdwarf-2 -fvar-tracking -fvar-tracking-assignments -O0 -ggdb -Werror main.c -o dcc_fs_shell
clean:	dcc_fs_shell
	rm dcc_fs_shell