all:
	gcc main.c -o main -Wall -Werror -lm -fsanitize=address,leak
