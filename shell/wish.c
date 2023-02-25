#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

struct Cmd {
    char **args;
    int nargs;
};

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void run_external(char* cmd, char **args, int nargs, FILE* fo) {
    int rc = fork();
    if (rc < 0) {
        // printf("fork failed\n");
        print_error();
    } else if (rc == 0) {
        // printf("child (pid:%d)\n", (int) (getpid()));

        char *myargs[nargs+1];
        
        for (int i=0; i<nargs+1; i++) {
            myargs[i] = NULL;
        }
        for (int j=0; j<nargs; j++) {
            myargs[j] = strdup(args[j]);
        }

        myargs[nargs] = NULL;

        if (fo != NULL) {
            dup2(fileno(fo), STDOUT_FILENO);
        }

        execv(cmd, myargs);

        for (int j=0; j<nargs+1; j++) {
            free(myargs[j]);
        }
    
    } else {
        // int rc_wait = 
        wait(NULL);
        // printf("parent of %d (rc_wait:%d) (pid:%d)\n", rc, rc_wait, (int) getpid());
    }
}

void parse_args(char *line, int nread, const char* delim, struct Cmd *cmd) {
    char *token = NULL;
    char *copy, *original_copy;
    copy = original_copy = strdup(line);
    copy[nread-1] = '\0';

    while ((token = strsep(&copy, delim)) != NULL) {
        if (strlen(token) != 0) {
            cmd->args = realloc(cmd->args, (cmd->nargs+1)*8);
            cmd->args[cmd->nargs] = strdup(token);
            cmd->nargs = cmd->nargs+1;
        }
    }
    free(original_copy);
}

void free_cmd(struct Cmd *cmd) {
    for (int i=0; i<cmd->nargs; i++) {
        free(cmd->args[i]);
    }
    free(cmd->args);
}

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread = 0;

    char *cline = malloc(8);
    cline[0] = '\0';

    struct Cmd maincmd = { malloc(8), 0 };
    struct Cmd delimcmd = { malloc(8), 0 };

    char exit_cmd[] = "exit";
    char cd_cmd[] = "cd";
    char path_cmd[] = "path";

    char **paths = malloc(sizeof(paths));
    paths[0] = strdup("/bin");
    int npaths = 1;

    FILE* fp = NULL;
    FILE* fo = NULL;

    auto void clean_exit();
    void clean_exit(int code) {
        for (int i=0; i<npaths; i++) { free(paths[i]); }
        free_cmd(&maincmd);
        free_cmd(&delimcmd);
        free(line);
        free(cline);
        free(paths);
        if (fp != NULL) {
            fclose(fp);
        }
        if (fo != 0) {
            fclose(fo);
        }
        exit(code);
    }

    // check if inputs come from stdin or a file

    if (argc==2) {
        fp = fopen(argv[1], "r");
            
        if (fp==NULL) {
            print_error();
            clean_exit(1);
        }

    } else if (argc>2) {
        print_error();
        clean_exit(1);
    }

    while(1) {
        if (!fp) {
            printf("wish> ");
            nread = getline(&line, &len, stdin);
        } else {
            nread = getline(&line, &len, fp);
        }
        if (nread == -1) {
            clean_exit(0);
        }

        int nonwhitespace = 0;
        for (int i=0; i<nread-1; i++) {
            if (line[i] != ' ') {
                nonwhitespace = 1;
            } 
        }

        // move on if receive a blank line
        if (nonwhitespace == 0) {
            continue;
        }

        // initially set cline to line
        cline = realloc(cline, nread+1);
        memcpy(cline, line, nread+1);
        cline[nread] = '\0';

        // check for redirection
        int count = 0;
        for (int i=0; i<strlen(line); i++) {
            if (line[i] == '>') {
                count ++;
            }
        }

        // only one redirection symbol allowed
        if (count > 1) {
            print_error();
            continue;
        } else if (count == 1) {
            parse_args(line, nread, ">", &delimcmd);
            if (delimcmd.nargs == 1) {
                print_error();
                continue;
            } else {
                cline = realloc(cline, strlen(delimcmd.args[0])+1);
                memcpy(cline, delimcmd.args[0], strlen(delimcmd.args[0]));
                cline[strlen(delimcmd.args[0])] = '\0';
                nread = strlen(delimcmd.args[0])+1;

                // delimcmd.args[1][strlen(delimcmd.args[1])] = '\0';
                // printf("delimcmd.args[1] = %s\n", delimcmd.args[1]);

                struct Cmd rightcmd = { malloc(8), 0 };
                parse_args(delimcmd.args[1], strlen(delimcmd.args[1])+1, " \t", &rightcmd);
                // printf("rightcmd.args[0] is '%s'; nargs %i\n", rightcmd.args[0], rightcmd.nargs);
                if (rightcmd.nargs>=2) {
                    free_cmd(&rightcmd);
                    print_error();
                    continue;
                }

                // try to open the output file
                fo = fopen(rightcmd.args[0], "w+");
                if (fo == NULL) {
                    print_error();
                    continue;
                }

                free_cmd(&rightcmd);
            } 
        }

        parse_args(cline, nread, " \t", &maincmd);

        // built-in commands
        if (strncmp(maincmd.args[0], exit_cmd, strlen(exit_cmd)) == 0) {
            if (maincmd.nargs > 1) {
                print_error();
            } else {
                clean_exit(0);
            }
        } else if (strncmp(maincmd.args[0], cd_cmd, strlen(cd_cmd)) == 0) {
            if (maincmd.nargs != 2) {
                print_error();
            } else {
                int result = chdir(maincmd.args[1]);
                if (result==-1) {
                    print_error();
                }
            }
        } else if (strncmp(maincmd.args[0], path_cmd, strlen(path_cmd)) == 0) {
            int n = maincmd.nargs-1;

            for (int i=0; i<npaths; i++) {
                free(paths[i]);
            }

            paths = realloc(paths, n * sizeof(*paths));
            for (int i=0; i<n; i++) {
                paths[i] = strdup(maincmd.args[i+1]);
            }

            npaths = n;
        }  
        
        else {
            // check for parallel commands
            struct Cmd cmds = { malloc(8), 0 };
            parse_args(cline, nread, "&", &cmds);


            for (int p=0; p<cmds.nargs; p++) {

                struct Cmd pcmd = { malloc(8), 0 };
                parse_args(cmds.args[p], strlen(cmds.args[p])+1, " \t", &pcmd);

                // loop over paths, print error message if not found anywhere
                int found_exec = 0;
                for (int j=0; j<npaths; j++) {
                    int pathlen = strlen(paths[j]) + strlen(pcmd.args[0]) + 2;
                    char* expath = malloc(8*pathlen); // TODO: Cause for leak?
                    sprintf(expath, "%s/%s", paths[j], pcmd.args[0]);

                    int result = access(expath, X_OK);
                    if (result==0) {
                        found_exec = 1;
                        run_external(expath, pcmd.args, pcmd.nargs, fo);
                        free(expath);
                        break;
                    }

                    free(expath);
                }
                
                if (!found_exec) {
                    print_error();
                }

                free_cmd(&pcmd);
            }

            free_cmd(&cmds);
        }

        // free memory between commands
        // printf("freeing memory\n");

        for (int i=0; i<maincmd.nargs; i++) {
            free(maincmd.args[i]);
        }
        maincmd.nargs = 0;

        for (int i=0; i<delimcmd.nargs; i++) {
            free(delimcmd.args[i]);
        }
        delimcmd.nargs = 0;

        cline[0] = '\0';

        if (fo != 0) {
            fclose(fo);
            fo = 0;
        }
    }
    
    return 0;
}