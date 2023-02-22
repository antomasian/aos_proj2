all:
	gcc -Wall -Werror wish.c -o wish -fsanitize=address -static-libasan -g
clean:
	rm wish
	rm -rf tests-out/
