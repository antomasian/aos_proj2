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

int run_external(char* cmd, char **args, int nargs, FILE* fo) {
    // printf("call to run external\n");

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
            // printf("fo not null");
            dup2(fileno(fo), STDOUT_FILENO);
            close(fileno(fo));
        }

        execvp(cmd, myargs);

        for (int j=0; j<nargs+1; j++) {
            free(myargs[j]);
        }

        // if (fo != 0) {
        //     fclose(fo);
        //     fo = 0;
        // }
    
    } 

    return rc;
    // else {
    //     // int rc_wait = 
    //     // if (last) {
    //         wait(NULL);
    //     // }
    //     // printf("parent of %d (rc_wait:%d) (pid:%d)\n", rc, rc_wait, (int) getpid());
    // }
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

    char exit_cmd[] = "exit";
    char cd_cmd[] = "cd";
    char path_cmd[] = "path";

    char **paths = malloc(sizeof(paths));
    paths[0] = strdup("/bin");
    int npaths = 1;

    int* rcs = malloc(sizeof(int));
    int n = 0;

    FILE* fp = NULL;

    auto void clean_exit();
    void clean_exit(int code) {
        for (int i=0; i<npaths; i++) { free(paths[i]); }
        free(line);
        free(paths);
        if (fp != NULL) {
            fclose(fp);
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

        n=0;

        // either read from stdin or file
        if (!fp) {
            printf("wish> ");
            nread = getline(&line, &len, stdin);
        } else {
            nread = getline(&line, &len, fp);
        }
        // if EOF exit
        if (nread == -1) {
            clean_exit(0);
        }

        // move on if receive a blank line
        int nonwhitespace = 0;
        for (int i=0; i<nread-1; i++) {
            if (line[i] != ' ') {
                nonwhitespace = 1;
            } 
        }
        if (nonwhitespace == 0) {
            continue;
        }

        struct Cmd cmds = { malloc(8), 0 };
    
        parse_args(line, nread, "&", &cmds);           

        // int rcs[cmds.nargs];

        // loop over commands
        for (int j=0; j<cmds.nargs; j++) {

            struct Cmd maincmd = { malloc(8), 0 };

            struct Cmd delimcmd = { malloc(8), 0 };

            FILE* fo = NULL;

            // check for redirects
            int count = 0;
            for (int i=0; i<strlen(cmds.args[j]); i++) {
                if (cmds.args[j][i] == '>') {
                    count ++;
                }
            }

            char *cline = malloc(8);
            cline[0] = '\0';

            // only one redirection symbol allowed
            if (count > 1) {
                free_cmd(&delimcmd);
                free(cline);
                free_cmd(&maincmd);
                print_error();
                continue;
            } else if (count == 1) {
    
                parse_args(cmds.args[j], strlen(cmds.args[j])+1, ">", &delimcmd);

                if (delimcmd.nargs == 1) {
                    print_error();
                    free_cmd(&maincmd);
                    free(cline);
                    free_cmd(&delimcmd);
                    continue;
                } else {
                    cline = realloc(cline, strlen(delimcmd.args[0])+1);
                    memcpy(cline, delimcmd.args[0], strlen(delimcmd.args[0]));
                    cline[strlen(delimcmd.args[0])] = '\0';
                    nread = strlen(delimcmd.args[0])+1;

                    delimcmd.args[1][strlen(delimcmd.args[1])] = '\0';

                    // make sure only one output file is provided
                    struct Cmd rightcmd = { malloc(8), 0 };
                    parse_args(delimcmd.args[1], strlen(delimcmd.args[1])+1, " \t", &rightcmd);
                    if (rightcmd.nargs>=2) {
                        // printf("here");
                        free_cmd(&rightcmd);
                        free_cmd(&delimcmd);
                        free(cline);
                        free_cmd(&maincmd);
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

            if (cline[0] != '\0') {
                parse_args(cline, nread, " \t", &maincmd);
                // free(cline);
            } else {
                parse_args(cmds.args[j], strlen(cmds.args[j])+1, " \t", &maincmd);
                // free(cline);
            }

            // printf("maincmd is %s\n", maincmd.args[0]);

            // built-in commands
            if (strncmp(maincmd.args[0], exit_cmd, strlen(exit_cmd)) == 0) {
                if (maincmd.nargs > 1) {
                    print_error();
                } else {
                    free_cmd(&delimcmd);
                    free_cmd(&maincmd);
                    free_cmd(&cmds);
                    free(cline);
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
            } else {
                // loop over paths, print error message if not found anywhere
                int found_exec = 0;
                for (int m=0; m<npaths; m++) {
                    int pathlen = strlen(paths[m]) + strlen(maincmd.args[0]) + 2;
                    char* expath = malloc(8*pathlen); // TODO: Cause for leak?
                    sprintf(expath, "%s/%s", paths[m], maincmd.args[0]);

                    int result = access(expath, X_OK);
                    if (result==0) {
                        found_exec = 1;
                        int rc = run_external(expath, maincmd.args, maincmd.nargs, fo);
                        // if (rc>0) {
                        //     wait(NULL);
                        // }
                        // rcs[j] = rc;
                        n++;
                        rcs = realloc(rcs, n*sizeof(int));
                        rcs[n-1] = rc;

                        free(expath);
                        break;
                    }

                    free(expath);
                }
                
                if (!found_exec) {
                    print_error();
                }
            }


            free_cmd(&maincmd);
            free_cmd(&delimcmd);

            if (fo != 0) {
                fclose(fo);
                fo = 0;
            }

            free(cline);
        }

        // free_cmd(&delimcmd);
        free_cmd(&cmds);

        for (int i=0; i<n; i++) {
            // wait(NULL);
            // printf("waiting %i", i);
            int status;
            waitpid(rcs[i], &status, 0);
        }

    }
    
    return 0;
}