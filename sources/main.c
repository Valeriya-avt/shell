#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <wait.h>

// pid_t child_pid = 0;

int num_of_bg = 0, process_num = 0;
pid_t *bg_pids = NULL, *pids = NULL;

#define PURPLE "\x1b[1;35m"
#define BLUE "\x1b[1;36m"
#define SEPARATOR_DESIGN "\x1b[0m"

void cmd_line_design() {
    char *user = getenv("USER");
    char *working_directory = getenv("PWD");
    printf(BLUE "%s" SEPARATOR_DESIGN ":" PURPLE "%s" SEPARATOR_DESIGN "$ ", user, working_directory);
    fflush(stdout);
}

char *get_word(char *end) {
    char *word = NULL, alpha;
    int n = 0;
    if (read(0, &alpha, sizeof(char)) < 0)
        perror("read");
    while (alpha != ' ' && alpha != '\t' && alpha != '\n') {
        word = realloc(word, (n + 1) * sizeof(char));
        word[n] = alpha;
        n++;
        if (read(0, &alpha, sizeof(char)) < 0)
            perror("read");
    }
    word = realloc(word, (n + 1) * sizeof(char));
    word[n] = '\0';
    *end = alpha;
    return word;
}

char **get_list(char *final_symbol, int *old_pipe_num) {
    char **list = NULL;
    char pipeline_word[] = "&&", pipe_conv_symbol[] = "|";
    char end;
    int i = 0, pipeline_flag = 0, new_pipe_num = *old_pipe_num;
    do {
        list = realloc(list, (i + 1) * sizeof(char *));
        list[i] = get_word(&end);
        if (!strcmp(list[i], pipeline_word)) {
            *final_symbol = end;
            pipeline_flag = 1;
        }
        if (!strcmp(list[i], pipe_conv_symbol)) {
            *final_symbol = end;
            new_pipe_num++;
        }
        i++;
    } while ((end != '\n') && (!pipeline_flag) && (new_pipe_num == *old_pipe_num));
    if (pipeline_flag || new_pipe_num != *old_pipe_num) {
        free(list[i - 1]);
        list[i - 1] = NULL;
        *final_symbol = '\0';
        *old_pipe_num = new_pipe_num;
        return list;
    }
    list = realloc(list, (i + 1) * sizeof(char *));
    list[i] = NULL;
    *final_symbol = end;
    return list;
}

char ***get_cmd_list(int *str_num, int *pipe_num) {
    int i = 0;
    char ***cmd_io_array = NULL;
    char end;
    *pipe_num = 0;
    do {
        cmd_io_array = realloc(cmd_io_array, (i + 1) * sizeof(char **));
        cmd_io_array[i] = get_list(&end, pipe_num);
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

// void clear_cmd(char **cmd) {
//     int i;
//     for (i = 0; cmd[i] != NULL; i++)
//         free(cmd[i]);
//     free(cmd[i]);
//     free(cmd);
// }

void change_descriptors(char **list, int output_fd, int input_fd, int output_index, int input_index) {
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
}

int open_file(char **list, int *fd, int output_flag, int input_flag, int index) {
    if (input_flag > 1 || output_flag > 1) {
        close(*fd);
        puts("Is failed");
        exit(1);
    }
    if (output_flag) {
        *fd = open(list[index + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    }
    else {
        *fd = open(list[index + 1], O_RDONLY);
    }
    return index;
}

void check_symbols(char **list, int *output, int *input) {
    int n = 0, output_flag = 0, input_flag = 0;
    int input_index = -1, output_index = -1; // изначально символы < > не найдены
    char output_symbol[] = ">", input_symbol[] = "<"; // bg_symbol[] = "&"; // сохраним символы, которые будем искать
    int output_fd = *output, input_fd = *input;
    while (list[n] != NULL && output_flag >= 0 && input_flag >= 0) { // будем бежать до конца строки
        if (!strcmp(list[n], output_symbol) && list[n + 1] != NULL) { // если нашли знак >
            output_flag++;
            output_index = open_file(list, &output_fd, output_flag, input_flag, n);
        }
        if (!strcmp(list[n], input_symbol) && list[n + 1] != NULL) { // если нашли знак <
            input_flag++;
            input_index = open_file(list, &input_fd, output_flag, input_flag, n);
        }
        n++;
    }
    if (input_flag || output_flag) {
        change_descriptors(list, output_fd, input_fd, output_index, input_index);
    }
}

void change_directory(char *old_path, char *new_dir) {
    char *new_path = NULL;
    if (chdir(new_dir)) {
        perror("cd");
        return;
    }
    new_path = getcwd(new_path, strlen(old_path) + strlen(new_dir) + 2);
    setenv("OLDPWD", old_path, 1); // обновляем путь до предыдущей директории
    setenv("PWD", new_path, 1);
    free(new_path);
}

int cd_command(char **list) {
    char *home = getenv("HOME"); // запомнили домашнюю директорию
    char *old_path = getenv("PWD");
    if (!strcmp((list[0]), "cd")) {
        if (list[1] == NULL || !strcmp(list[1], "~")) {
            change_directory(old_path, home);
        } else {
            if (!strcmp(list[1], "-")) {
                char *prev_dir = getenv("OLDPWD");
                if (prev_dir == NULL)
                    perror("cd");
                else
                    change_directory(old_path, prev_dir);
            } else {
                change_directory(old_path, list[1]);
            }
        }
        return 0;
    }
    return 1;
}

void run_background(char **list) {
    bg_pids = realloc(bg_pids, (num_of_bg + 1) * sizeof(pid_t));
    free(list[1]);
    list[1] = NULL;
    bg_pids[num_of_bg] = fork();
    if (bg_pids[num_of_bg] == 0) {
        printf("[%d] %u", num_of_bg + 1, getpid());
        execvp(list[0], list);
        return;
    }
    num_of_bg++;
    return;
}

int background_process(char **list) {
    if (list[1] != NULL && !strcmp(list[1], "&")) {
        run_background(list);
        return 1;
    }
    return 0;
}

void create_pipes(char ***list, int pipes) { // в pipes число процессов в конвейере
    pid_t pids[10];
    int (*pipefd)[2], i;
    pipefd = malloc((pipes - 1) * sizeof(int [2])); // для n процессов (n - 1) pipe
    for (i = 0; i < pipes; ++i) {
        if (i != (pipes - 1)) {
            pipe(pipefd[i]);
        }
        pids[i] = fork();
        if (pids[i] == 0) {
            if (i != 0) {
                dup2(pipefd[i - 1][0], 0); // читаем из предыдущего pipe
                if (i == pipes - 1) {
                    check_symbols(list[i], &pipefd[i - 1][1], &pipefd[i - 1][0]);
                }
                close(pipefd[i - 1][1]);
                close(pipefd[i - 1][0]);
            }
            if (i != (pipes - 1)) {
                dup2(pipefd[i][1], 1); // пишем в текущий pipe
                if (!i) {
                    check_symbols(list[i], &pipefd[i][1], &pipefd[i][0]);
                }
                close(pipefd[i][0]);
                close(pipefd[i][1]);
            }
            for (int j = 0; j < i - 1; j++) {
                close(pipefd[j][0]);
                close(pipefd[j][1]);
            }
            execvp(list[i][0], list[i]);
        }
    }
    for (i = 0; i < pipes; i++) {
        if (i != (pipes - 1)) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }
        waitpid(pids[i], NULL, 0);
        pids[i] = 0;
    }
    free(pipefd);
}

void pipeline_of_commands(char ***list, int cmd_num) {
    int i, input_fd, output_fd, child_status;
    for (i = 0; i < cmd_num; i++) {
        if (!cd_command(list[i]))
            return;
        if (background_process(list[i]))
            return;
        input_fd = 0;
        output_fd = 1;
        if (fork() == 0) {
            check_symbols(list[i], &output_fd, &input_fd); // в дочернем процессе ищем знаки < > в строке
            if (execvp(list[i][0], list[i]) < 0) {
                if (strcmp(list[i][0], "\0"))
                    perror(list[i][0]);
             //   clear_list(list);
                exit(1);
            }
        } else {
            wait(&child_status);
            if (child_status != 0)
                return;
        }
    }
}

char ***run_commands(char ***list, int str_num, int pipe_num) {
    if (str_num > 1 && pipe_num > 0) {
        create_pipes(list, str_num);
    } else {
        pipeline_of_commands(list, str_num);
    }
    return list;
}

void check_input(char ***list, int *str_num, int *pipe_num) {
    while(list[0][0] == NULL) {
        clear_list(list);
        cmd_line_design();
        list = get_cmd_list(str_num, pipe_num);
    }
}

int main(int argc, char **argv) {
    int i, str_num, pipe_num;
    cmd_line_design();
    char ***list = get_cmd_list(&str_num, &pipe_num);
    char finish1[] = "exit", finish2[] = "quit";
    check_input(list, &str_num, &pipe_num);
    while (strcmp(list[0][0], finish1) && strcmp(list[0][0], finish2)) { // делаем fork, пока не встретим exit или quit
         list = run_commands(list, str_num, pipe_num);
         clear_list(list);
         cmd_line_design();
         list = get_cmd_list(&str_num, &pipe_num);
         check_input(list, &str_num, &pipe_num);
   }
    clear_list(list); //удаляем строку с exit или quit
    for (i = 0; i < num_of_bg; i++) {
        waitpid(bg_pids[i], NULL, 0);
    }
    free(bg_pids);
    return 0;
}
