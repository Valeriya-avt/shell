#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <wait.h>
#include <signal.h>

int num_of_bg = 0, num_of_processes = 0;
pid_t *bg_pids = NULL, *pids = NULL;

#define PURPLE "\x1b[1;35m"
#define BLUE "\x1b[1;36m"
#define SEPARATOR_DESIGN "\x1b[0m"

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
    free(pids);
    pids = NULL;
    num_of_processes = 0;
}

void duplicate_fd(char **list, int index, int old_fd, int new_fd) {
    dup2(old_fd, new_fd);
    free(list[index + 1]);
    free(list[index]);
    list[index] = NULL;
}

void change_descriptors(char **list, int output_fd, int input_fd, int output_index, int input_index) {
    if (input_index > 0) // if found <
        duplicate_fd(list, input_index, input_fd, 0);
    if (output_index > 0) // if found >
        duplicate_fd(list, output_index, output_fd, 1);
}

int open_file(char **list, int *fd, int output_flag, int input_flag, int index, int flag) {
    if (input_flag > 1 || output_flag > 1) {
        close(*fd);
        puts("Is failed");
        exit(1);
    }
    if (flag) {
        *fd = open(list[index + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    } else {
        *fd = open(list[index + 1], O_RDONLY);
        if (*fd < 0) {
            perror(list[index + 1]);
            exit(1);
        }
    }
    return index;
}

void check_io_symbols(char **list, int *output, int *input) {
    int n = 0, output_flag = 0, input_flag = 0;
    int input_index = -1, output_index = -1; // initially < > not found
    char output_symbol[] = ">", input_symbol[] = "<";
    int output_fd = *output, input_fd = *input;
    while (list[n] != NULL) {
        if (!strcmp(list[n], output_symbol) && list[n + 1] != NULL) { // if found >
            output_flag++;
            output_index = open_file(list, &output_fd, output_flag, input_flag, n, 1);
        }
        if (!strcmp(list[n], input_symbol) && list[n + 1] != NULL) { // if found <
            input_flag++;
            input_index = open_file(list, &input_fd, output_flag, input_flag, n, 0);
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
    setenv("OLDPWD", old_path, 1); // update the path to the previous directory
    setenv("PWD", new_path, 1);
    free(new_path);
}

int cd_command(char **list) {
    char *home = getenv("HOME");
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
        return 1;
    }
    return 0;
}

void run_background(char **list, int index) {
    bg_pids = realloc(bg_pids, (num_of_bg + 1) * sizeof(pid_t));
    free(list[index]);
    list[index] = NULL;
    bg_pids[num_of_bg] = fork();
    if (bg_pids[num_of_bg] == 0) {
        execvp(list[0], list);
        perror(list[0]);
        return;
    }
    num_of_bg++;
    return;
}

int background_process(char **list) {
    int i;
    for (i = 0; list[i] != NULL; i++) {
        if (list[1] != NULL && !strcmp(list[i], "&")) {
            run_background(list, i);
            return 1;
        }
    }
    return 0;
}

void create_pipes(char ***list, int pipes) { // pipes - number of processes in the pipeline
    int (*pipefd)[2], i;
    pipefd = malloc((pipes - 1) * sizeof(int [2])); // number of pipes for n processes is (n - 1)
    for (i = 0; i < pipes; ++i) {
        if (i != (pipes - 1)) {
            pipe(pipefd[i]);
        }
        pids = realloc(pids, (num_of_processes + 1) * sizeof(pid_t));
        pids[num_of_processes] = fork();
        if (pids[num_of_processes] == 0) {
            if (i != 0) {
                dup2(pipefd[i - 1][0], 0); // read from previous pipe
                if (i == pipes - 1) {
                    check_io_symbols(list[i], &pipefd[i - 1][1], &pipefd[i - 1][0]);
                }
                close(pipefd[i - 1][1]);
                close(pipefd[i - 1][0]);
            }
            if (i != (pipes - 1)) {
                dup2(pipefd[i][1], 1);
                if (!i) {
                    check_io_symbols(list[i], &pipefd[i][1], &pipefd[i][0]);
                }
                close(pipefd[i][0]);
                close(pipefd[i][1]);
            }
            for (int j = 0; j < i - 1; j++) {
                close(pipefd[j][0]);
                close(pipefd[j][1]);
            }
            execvp(list[i][0], list[i]);
            perror(list[i][0]);
            exit(1);
        } else {
            for (int j = 0; j < i; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
            }
        }
        num_of_processes++;
    }
    close(pipefd[pipes - 2][0]);
    close(pipefd[pipes - 2][1]);
   for (i = 0; i < pipes; i++) {
       waitpid(pids[i], NULL, 0);
       pids[i] = 0;
   }
    free(pipefd);
}

void pipeline_of_commands(char ***list, int cmd_num) {
    int i, input_fd, output_fd, child_status;
    for (i = 0; i < cmd_num; i++) {
        if (cd_command(list[i]))
            return;
        if (background_process(list[i]))
            return;
        input_fd = 0;
        output_fd = 1;
        pids = realloc(pids, (num_of_processes + 1) * sizeof(pid_t));
        pids[num_of_processes] = fork();
        if (pids[num_of_processes] == 0) {
            check_io_symbols(list[i], &output_fd, &input_fd);
            if (execvp(list[i][0], list[i]) < 0) {
                if (strcmp(list[i][0], "")) // if not '\n'
                    perror(list[i][0]);
                exit(1);
            }
        } else {
            num_of_processes++;
            wait(&child_status);
            pids[i] = 0;
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

void cmd_line_design() {
    char *user = getenv("USER");
    char *working_directory = getenv("PWD");
    printf(BLUE "%s" SEPARATOR_DESIGN ":" PURPLE "%s" SEPARATOR_DESIGN "$ ", user, working_directory);
    fflush(stdout); // forces a write of all user-space buffered data in stdout
}

void check_input(char ***list, int *str_num, int *pipe_num) {
    while(list[0][0] == NULL) {
        clear_list(list);
        cmd_line_design();
        list = get_cmd_list(str_num, pipe_num);
    }
}

void handler(int signo) {
    int i;
    putchar('\n');
    cmd_line_design();
    for (i = 0; i < num_of_processes; i++) {
        printf("process pid %u\n", pids[i]);
        if (pids[i]) {
            printf("kill %u\n", pids[i]);
            kill(pids[i], SIGINT);
        }
        waitpid(pids[i], NULL, 0);
        pids[i] = 0;
    }
}

int main(int argc, char **argv) {
    int i, str_num, pipe_num;
    signal(SIGINT, handler);
    cmd_line_design();
    char ***list = get_cmd_list(&str_num, &pipe_num);
    char finish1[] = "exit", finish2[] = "quit";
    check_input(list, &str_num, &pipe_num);
    while (strcmp(list[0][0], finish1) && strcmp(list[0][0], finish2)) {
         list = run_commands(list, str_num, pipe_num);
         clear_list(list);
         cmd_line_design();
         list = get_cmd_list(&str_num, &pipe_num);
         check_input(list, &str_num, &pipe_num);
   }
    clear_list(list); // delete list with exit or quit
    for (i = 0; i < num_of_bg; i++) {
        waitpid(bg_pids[i], NULL, 0);
    }
    free(bg_pids);
    return 0;
}
