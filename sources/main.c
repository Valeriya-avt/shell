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
    if (read(0, &alpha, sizeof(char)) < 0)
        perror("Is failed");
    while (alpha != ' ' && alpha != '\t' && alpha != '\n') {
        word = realloc(word, (n + 1) * sizeof(char));
        word[n] = alpha;
        n++;
        if (read(0, &alpha, sizeof(char)) < 0)
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

char ***get_cmd_list(int *str_num) {
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
    *str_num = i;
    return cmd_io_array;
}

void clear_list(char ***list) {
    int i, j;
    for (i = 0; list[i] != NULL; i++) {
        for (j = 0; list[i][j] != NULL; j++)
            free(list[i][j]);
        free(list[i][j]);
        free(list[i]);
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

void checking_descriptors(char **list, int output_fd, int input_fd, int fd, int n) {
    char *tmp;
    if (output_fd == fd) { // если нашли знак >
        dup2(fd, 1); // направили вывод программы в файл
        free(list[n]); // удалили элемент с названием файла
        tmp = list[n - 1]; //
        list[n - 1] = NULL; // конец строки указывает на NULL
        free(tmp); // удалили элемент со знаком >
    }
    if (input_fd == fd) {
        dup2(fd, 0); // теперь считывать будем из файла
        free(list[n]);
        tmp = list[n - 1];
        list[n - 1] = NULL;
        free(tmp);
    }
    if (execvp(list[0], list) < 0) {
        perror("Is failed");
        clear_cmd(list);
        return;
    }
    if (fd > 0) { // если открыли fd, закроем его
        close(fd);
    }

}

char **search_io_symbol(char **list, int *output, int *input) {
    int n = 0;
    int fd = -1; // изначально файл не открыт
    char output_symbol[] = ">", input_symbol[] = "<"; // сохраним символы, которые будем искать
    int output_fd = *output, input_fd = *input; // для удобства заведём локальные флаги
    while ((list[n] != NULL) && (output_fd == 1) && (input_fd == 0)) { // будем бежать по строке, пока не найдём < или >
        if (!strcmp(list[n], output_symbol) && list[n + 1] != NULL) { // если нашли знак >
            fd = open(list[n + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); // открыли файл на запись (считаем, что имя файла идёт сразу после >)
            output_fd = fd;
        }
        if (!strcmp(list[n], input_symbol) && list[n + 1] != NULL) { // если нашли знак <
            fd = open(list[n + 1], O_RDONLY); // открыли файл на чтение
            input_fd = fd; // выставили флаг
        }
        n++; // обновляем счётчик
    }
    checking_descriptors(list, output_fd, input_fd, fd, n);
    return list;
}

void create_pipe(char ***list) {
    int fd[2];
    pipe(fd);
    if (fork() == 0) {
        dup2(fd[1], 1);
        close(fd[0]);
        close(fd[1]);
        execvp(list[0][0], list[0]);
    }
    if (fork() == 0) {
        dup2(fd[0], 0);
        close(fd[0]);
        close(fd[1]);
        execvp(list[1][0], list[1]);
    }
    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
}

char ***run_commands(char ***list, int str_num) {
    int n = 0, output_fd, input_fd;
    if (str_num > 1) {
        create_pipe(list);
    } else {
        while (list[n] != NULL) {
            output_fd = 1;
            input_fd = 0;
            if (fork() > 0) { // родительский процесс ждёт завершения дочернего
                wait(NULL);
            } else {
                list[n] = search_io_symbol(list[n], &output_fd, &input_fd); // в дочернем процессе ищем знаки < > в строке
            }
            n++;
        }
    }
    return list;
}

int main(int argc, char **argv) {
    int str_num;
    char ***list = get_cmd_list(&str_num);
    char finish1[] = "exit", finish2[] = "quit";
    while (strcmp(list[0][0], finish1) && strcmp(list[0][0], finish2)) { // делаем fork, пока не встретим exit или quit
         list = run_commands(list, str_num);
         clear_list(list);
         list = get_cmd_list(&str_num);
   }
    clear_list(list); //удаляем строку с exit или quit
    return 0;
}
