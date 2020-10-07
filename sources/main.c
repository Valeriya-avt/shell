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

char **get_cmd() {
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
 //   *size = i;
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

int search_symbol(char **list, int *input, int *output, int *i) {
    int n = *i;
    int fd = -1; // изначально файл не открыт
    char input_symbol[] = ">", output_symbol[] = "<"; // сохраним символы, которые будем искать
    int input_flag = *input, output_flag = *output; // для удобства заведём локальные флаги
    while ((list[n] != NULL) && (!input_flag) && (!output_flag)) { // будем бежать по строке, пока не найдём < или >
        if (!strcmp(list[n], input_symbol) && list[n + 1] != NULL) { // если нашли знак >
            fd = open(list[n + 1], O_WRONLY | O_CREAT | O_TRUNC,
                 S_IRUSR | S_IWUSR); // открыли файл на запись (считаем, что имя файла идёт сразу после >)
            input_flag = 1; // выставили флаг
        }
        if (!strcmp(list[n], output_symbol) && list[n + 1] != NULL) { // если нашли знак <
            fd = open(list[n + 1], O_RDONLY); // октрыли файл на чтение
            output_flag = 1; // выставили флаг
        }
        n++; // обновляем счётчик
    }
    *input = input_flag; // передали состояние флагов
    *output = output_flag;
    *i = n; // в list[n] лежит название файла
    return fd; // вернули файловый дескриптор
}

int checking_flags(char **list, int input_flag, int output_flag, int fd, int n) {
    char *tmp;
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
        clear(list);
        return 1;
    }
    return 0;
}

void create_pipe(char **cmd1, char **cmd2) {
    int fd[2];
    pipe(fd);
    if (fork() == 0) {
        dup2(fd[1], 1);
        close(fd[0]);
        close(fd[1]);
        execvp(cmd1[0], cmd1);
    }
    if (fork() == 0) {
        dup2(fd[0], 0);
        close(fd[0]);
        close(fd[1]);
        execvp(cmd2[0], cmd2);
    }
    close(fd[0]);
    close(fd[1]);
    wait(NULL);
    wait(NULL);
}

int search_separator(char **list) {
    int n = 0;
    char separator_symbol[] = "|";
    while (list[n] != NULL) { // будем бежать по строке, пока не найдём |
        if (!strcmp(list[n], separator_symbol) && list[n + 1] != NULL) { // если нашли |
            char *cmd1[] = {list[0], NULL};
            char *cmd2[] = {list[2], NULL};
            create_pipe(cmd1, cmd2);
            return 1;
        }
        n++;
    }
    return 0;
}

// char ***get_list(char **list) {
//     int n = 0, i = 0;
//     char **array = list;
//     char ***cmd_io_array = NULL;
//     char **tmp = NULL;
//     char separator[] = "|";
//     while (list[n] != NULL) {
//         cmd_io_array = realloc(cmd_io_array, (i + 1) * sizeof(char **));
//         while (strcmp(list[n]), separator)) {
//             cmd_io_array[i] =
//             n++;
//         }
//         if (!strcmp(list[n], separator)) {
//
//         }
//         i++;
//     }
//     cmd_io_array[i] = NULL;
//     return cmd_io_array;
// }

int main(int argc, char **argv) {
    int n = 0, fd; //
    char **list = get_cmd();
//    char ***cmd_io_array = get_list(list);
    char finish1[] = "exit", finish2[] = "quit";
    int input_flag, output_flag, separator_flag;
    while (strcmp(list[0], finish1) && strcmp(list[0], finish2)) { // делаем fork, пока не встретим exit или quit
    //    print_list(list, size);
        n = 0; // ?
        input_flag = 0; // еще не встретили знак >
        output_flag = 0; // ещё не встретили знак <
        separator_flag = 0; // ещё не встретили |
        separator_flag = search_separator(list);
        if (!separator_flag) {
            if (fork() > 0) { // родительский процесс ждёт завершения дочернего
                wait(NULL);
            } else {
                fd = search_symbol(list, &input_flag, &output_flag, &n); // в дочернем процессе ищем знаки < > в строке
                    if (checking_flags(list, input_flag, output_flag, fd, n)) // проверяем, нашли ли мы < >
                        return 1; //если попали сюда - произошла ошибка при вызове exec в  checking_flags
            }
            if (input_flag || output_flag) { // если какой-либо из флагов выставлен, закрываем файловый дескриптор
                close(fd);
            }
        }
        clear(list);
        list = get_cmd();
    }
    clear(list); //удаляем строку с exit или quit
    return 0;
}
