#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int main(int argc, char *argv[]) {

    char *line = NULL;
    size_t len = 0;
    ssize_t nread = 0;

    char *token = NULL;
    char **args = malloc(sizeof(*args));

    char exit_cmd[] = "exit";
    char cd_cmd[] = "cd";
    char path_cmd[] = "path";

    while (1) {
        printf("wish> ");
        nread = getline(&line, &len, stdin);
        if (nread == -1) {
            exit(0);
        }

        int k = 0;
        while ((token = strsep(&line, " ")) != NULL) {
            char **tmp = realloc(args, (k + 1) * sizeof(*args));
            args = tmp;
            args[k] = token;
            k++;
        }

        // built-in commands
        if (strncmp(args[0], exit_cmd, strlen(exit_cmd)) == 0) {
            if (k > 1) {
                print_error();
            } else {
                exit(0);
            }
        } else if (strncmp(args[0], cd_cmd, strlen(cd_cmd)) == 0) {
            if (k != 2) {
                print_error();
            } else {

            }
        } else if (strncmp(args[0], path_cmd, strlen(path_cmd)) == 0) {
        
        } else {
            

        }

        for (int i = 0; i < k; i++) {
            free(args[i]);
        }
    }

    free(token);
    free(line);

    return 0;
}