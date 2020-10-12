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
 //       free(list[i][j]);
        free(list[i]);
    }
 //   free(list[i]);
    free(list);
}

void clear_cmd(char **cmd) {
    int i;
    for (i = 0; cmd[i] != NULL; i++)
        free(cmd[i]);
    free(cmd[i]);
    free(cmd);
}

void checking_descriptors(char **list, int output_fd, int input_fd, int output_index, int input_index) {
    char *tmp;
    if (output_index > 0) { // если нашли знак >
        dup2(output_fd, 1); // направили вывод программы в файл
        free(list[output_index + 1]); // удалили элемент с названием файла
        tmp = list[output_index];
        list[output_index] = NULL; // конец строки указывает на NULL
        free(tmp); // удалили элемент со знаком >
    }
    if (input_index > 0) {
        dup2(input_fd, 0); // теперь считывать будем из файла
        free(list[input_index + 1]);
        tmp = list[input_index];
        list[input_index] = NULL;
        free(tmp);
    }
    if (execvp(list[0], list) < 0) {
        perror("Is failed");
        clear_cmd(list);
        return;
    }
    if (output_fd > 0) // если открыли fd, закроем его
        close(output_fd);
    if (input_fd > 0)
        close(input_fd);
}

char **search_io_symbol(char **list, int *output, int *input) {
    int n = 0, output_flag = 0, input_flag = 0;
 //   int fd = -1; // изначально файл не открыт
    int input_index = -1, output_index = -1; // изначально символы < > не найдены
    char output_symbol[] = ">", input_symbol[] = "<"; // сохраним символы, которые будем искать
    int output_fd = *output, input_fd = *input; // для удобства заведём локальные флаги
    while (list[n] != NULL) { // будем бежать до конца строки
        if (!strcmp(list[n], output_symbol) && list[n + 1] != NULL) { // если нашли знак >
            if (output_flag) {
                close(output_fd);
                puts("Is failed");
                exit(1);
            }
            output_fd = open(list[n + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); // открыли файл на запись (считаем, что имя файла идёт сразу после >)
            output_index = n;
            output_flag++;
        }
        if (!strcmp(list[n], input_symbol) && list[n + 1] != NULL) { // если нашли знак <
            if (input_flag) {
                close(input_fd);
                puts("Is failed");
                exit(1);
            }
            input_fd = open(list[n + 1], O_RDONLY); // открыли файл на чтение
            input_index = n;
            input_flag++;
        }
        n++;
    }
    checking_descriptors(list, output_fd, input_fd, output_index, input_index);
    return list;
}

void create_pipe(char ***list) {
    int fd[2];
    pipe(fd);
    if (fork() == 0) { // направим поток вывода первого процесса в pipe
        dup2(fd[1], 1);
        close(fd[0]);
        close(fd[1]);
        execvp(list[0][0], list[0]);
    }
    if (fork() == 0) { // будем читать из pipe
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

void create_pipes(char ***list, int pipes) { // в pipes число процессов в конвейере
    pid_t pids[10];
    int i, (*pipefd)[2];
    pipefd = malloc((pipes - 1) * sizeof(int [2])); // для n процессов (n - 1) pipe // [3,4]   []>pipe>[]
    for (i = 0; i < pipes; i++) {
        if (i != (pipes - 1)) {
            pipe(pipefd[i]); // [parent4]>pipe>parent3
            printf("open %d %d\n", pipefd[i][0], pipefd[i][1]);
        }
        pids[i] = fork(); // child4, parent4 >pipe> child3, parent3
        if (pids[i] == 0) {
            if (i != 0) {
                dup2(pipefd[i - 1][0], 0); // читаем из предыдущего pipe
//                printf("close in for1.1 %d %d\n", pipefd[i - 1][0], pipefd[i - 1][1]);
                close(pipefd[i - 1][0]);
                close(pipefd[i - 1][1]);
            }
            if (i != (pipes - 1)) {
                dup2(pipefd[i][1], 1); // пишем в текущий pipe
//                printf("close in for1.2 %d %d\n", pipefd[i][0], pipefd[i][1]);
                close(pipefd[i][0]);
                close(pipefd[i][1]);
            }
            puts("list[i][0]");
            execvp(list[i][0], list[i]);
        }
    }
    for (i = 0; i < pipes; i++) {
   //     if (pids[i] > 0) {
            if (i != 0) {
 //               printf("close in for2 %d %d\n", pipefd[i - 1][0], pipefd[i - 1][1]);
                close(pipefd[i - 1][0]);
                close(pipefd[i - 1][1]);
            }
            if (i != (pipes - 1)) {
 //               printf("close in for2 %d %d\n", pipefd[i][0], pipefd[i][1]);
                close(pipefd[i][0]);
                close(pipefd[i][1]);
            }
   //     }
    //    if (pids[i] > 0) {
 //           printf("wait %d\n", pids[i]);
            waitpid(pids[i], NULL, 0);
    //    }
    }
    free(pipefd);
}

char ***run_commands(char ***list, int str_num) {
    int n = 0, output_fd, input_fd;
    if (str_num > 1) { // && !flag
        create_pipes(list, str_num);
  //      create_pipe(list);
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
