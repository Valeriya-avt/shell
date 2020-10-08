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

char **get_list(char *final_symbol) {
    char **list = NULL;
    char separator[] = "|";
    char end;
    int i = 0, separator_flag = 0;
    do {
        list = realloc(list, (i + 1) * sizeof(char *));
        list[i] = get_word(&end);
        if (!strcmp(list[i], separator)) {
            *final_symbol = end;
            separator_flag = 1;
        }
        i++;
    } while ((end != '\n') && (!separator_flag));
    if (separator_flag) {
        free(list[i - 1]);
        list[i - 1] = NULL;
        *final_symbol = '\0';
        return list;
    }
    list = realloc(list, (i + 1) * sizeof(char *));
    list[i] = NULL;
    *final_symbol = end;
    return list;
}

char ***get_cmd_list() {
    int i = 0;
    char ***cmd_io_array = NULL;
    char end;
    do {
        cmd_io_array = realloc(cmd_io_array, (i + 1) * sizeof(char **));
        cmd_io_array[i] = get_list(&end);
        i++;
    } while (end != '\n');
    cmd_io_array = realloc(cmd_io_array, (i + 1) * sizeof(char **));
    cmd_io_array[i] = NULL;
    return cmd_io_array;
}

//void print_list(char **list, int size) {
//    int i;
//    for (i = 0; i < size; i++) {
//        write(0, list[i], strlen(list[i]) * sizeof(char));
//        write(0, " ", sizeof(char));
//    }
//    putchar('\n');
//}

void clear_list(char ***list) {
    int i, j;
    for (i = 0; list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++)
            free(list[i][j]);
        free(list[i][j]);
    }
    free(list[i]);
    free(list);
}

void clear_cmd(char **cmd) {
    int i;
    for (i = 0; cmd[i] != NULL; i++)
        free(cmd[i]);
    free(cmd[i]);
    free(cmd);
}

char **search_io_symbol(char **list, int *input, int *output) {
    int n = 0;
    int fd = -1; // изначально файл не открыт
    char input_symbol[] = ">", output_symbol[] = "<"; // сохраним символы, которые будем искать
    char *tmp = NULL;
    int input_flag = *input, output_flag = *output; // для удобства заведём локальные флаги
    while ((list[n] != NULL) && (!input_flag) && (!output_flag)) { // будем бежать по строке, пока не найдём < или >
        if (!strcmp(list[n], input_symbol) && list[n + 1] != NULL) { // если нашли знак >
            fd = open(list[n + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); // открыли файл на запись (считаем, что имя файла идёт сразу после >)
            input_flag = 1;
        }
        if (!strcmp(list[n], output_symbol) && list[n + 1] != NULL) { // если нашли знак <
            fd = open(list[n + 1], O_RDONLY); // открыли файл на чтение
            output_flag = 1; // выставили флаг
        }
        n++; // обновляем счётчик
    }
    if (input_flag) { // если нашли знак >
        dup2(fd, 1); // направили вывод программы в файл
        free(list[n]); // удалили элемент с названием файла
        tmp = list[n - 1]; //
        list[n - 1] = NULL; // конец строки указывает на NULL
        free(tmp); // удалили элемент со знаком >
    }
    if (output_flag) {
        dup2(fd, 0); // теперь считывать будем из файла
        free(list[n]);
        tmp = list[n - 1];
        list[n - 1] = NULL;
        free(tmp);
    }
    if (execvp(list[0], list) < 0) {
        perror("Is failed");
        clear_cmd(list);
        return NULL;
    }
    if (fd > 0) { // если открыли fd, закроем его
        close(fd);
    }
    return list;
}

// int checking_flags(char **list, int input_flag, int output_flag, int fd, int n) {
//     char *tmp;
    // if (input_flag) { // если нашли знак >
    //     dup2(fd, 1); // направили вывод программы в файл
    //     free(list[n]); // удалили элемент с названием файла
    //     tmp = list[n - 1]; //
    //     list[n - 1] = NULL; // конец строки указывает на NULL
    //     free(tmp); // удалили элемент со знаком >
    // }
    // if (output_flag) {
    //     dup2(fd, 0); // теперь считывать будем из файла
    //     free(list[n]);
    //     tmp = list[n - 1];
    //     list[n - 1] = NULL;
    //     free(tmp);
    // }
    // if (execvp(list[0], list) < 0) {
    //     perror("Is failed");
    //     clear_cmd(list);
    //     return 1;
    // }
//     return 0;
// }

// void create_pipe(char **cmd1, char **cmd2) {
//     int fd[2];
//     pipe(fd);
//     if (fork() == 0) {
//         dup2(fd[1], 1);
//         close(fd[0]);
//         close(fd[1]);
//         execvp(cmd1[0], cmd1);
//     }
//     if (fork() == 0) {
//         dup2(fd[0], 0);
//         close(fd[0]);
//         close(fd[1]);
//         execvp(cmd2[0], cmd2);
//     }
//     close(fd[0]);
//     close(fd[1]);
//     wait(NULL);
//     wait(NULL);
// }

char ***run_commands(char ***list) {
    int n = 0, input_flag, output_flag;
    while (list[n] != NULL) {
        input_flag = 0;
        output_flag = 0;
        if (fork() > 0) { // родительский процесс ждёт завершения дочернего
            wait(NULL);
        } else {
            list[n] = search_io_symbol(list[n], &input_flag, &output_flag); // в дочернем процессе ищем знаки < > в строке
        }
        n++;
    }
    return list;
}

int main(int argc, char **argv) {
    char ***list = get_cmd_list();
    char finish1[] = "exit", finish2[] = "quit";
    while (strcmp(list[0][0], finish1) && strcmp(list[0][0], finish2)) { // делаем fork, пока не встретим exit или quit
        list = run_commands(list);
        clear_list(list);
        list = get_cmd_list();
    }
    clear_list(list); //удаляем строку с exit или quit
    return 0;
}
