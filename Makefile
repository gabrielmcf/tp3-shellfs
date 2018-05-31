make:	main.c
	gcc -g2 -gdwarf-2 -fvar-tracking -fvar-tracking-assignments -Og -ggdb -Werror main.c -o dcc_fs_shell -lext2fs -lcom_err
clean:	dcc_fs_shell
	rm dcc_fs_shell