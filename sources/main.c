#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <wait.h>

char *get_word(char *end) {
    char *word = NULL, alpha;
    int n = 0;
    if (read(1, &alpha, sizeof(char)) < 0)
        perror("Is failed");
    while (alpha != ' ' && alpha != '\t' && alpha != '\n') {
        word = realloc(word, (n + 1) * sizeof(char));
        word[n] = alpha;
        n++;
        if (read(1, &alpha, sizeof(char)) < 0)
            perror("Is failed");
    }
    word = realloc(word, (n + 1) * sizeof(char));
    word[n] = '\0';
    *end = alpha;
    return word;
}

char **get_list() {
    char **list = NULL;
    char end;
    int i = 0;
    do {
        list = realloc(list, (i + 1) * sizeof(char *));
        list[i] = get_word(&end);
        i++;
    } while (end != '\n');
    list = realloc(list, (i + 1) * sizeof(char *));
    list[i] = NULL;
//    *size = i;
    return list;
}

//void print_list(char **list, int size) {
//    int i;
//    for (i = 0; i < size; i++) {
//        write(0, list[i], strlen(list[i]) * sizeof(char));
//        write(0, " ", sizeof(char));
//    }
//    putchar('\n');
//}

void clear(char **list) {
    int i;
    for (i = 0; list[i] != NULL; i++) {
        free(list[i]);
    }
    free(list[i]);
    free(list);
}

int main(int argc, char **argv) {
    int n = 0, fd;
 //   int size;
    char **list = get_list(&size);
    char finish1[] = "exit", finish2[] = "quit";
    char input[] = ">", output[] = "<";
    int input_flag, output_flag;
    while (strcmp(list[0], finish1) && strcmp(list[0], finish2)) {
    //    print_list(list, size);
        input_flag = 0;
        output_flag = 0;
        if (fork() > 0) {
            wait(NULL);
        } else {
            while ((list[n] != NULL) && (!input_flag) && (!output_flag)) {
                if (!strcmp(list[n], input) && list[n + 1] != NULL) {
                    fd = open(list[n + 1], O_WRONLY | O_CREAT | O_TRUNC,
                         S_IRUSR | S_IWUSR);
                    input_flag = 1;
                }
                if (!strcmp(list[n], output) && list[n + 1] != NULL) {
                    fd = open(list[n + 1], O_RDONLY);
                    output_flag = 1;
                }
                n++;
            }
            if (input_flag) {
                dup2(fd, 1);
                free(list[n]);
                list[n - 1] = NULL;
            } 
            if (output_flag) {
                dup2(fd, 0);
                free(list[n]);
                list[n - 1] = NULL;
            }
            if (execvp(list[0], list) < 0) {
                perror("exec failed");
                return 1;
            }
        }
        clear(list);
        list = get_list(&size);
        if (input_flag || output_flag) {
            close(fd);
        }
    }
    clear(list);
    return 0;
}
