#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    const int MAXCMD = 20;
    const char error_message[30] = "An error has occurred\n";
    size_t bufsize = 2048;
    //path variables

    if (argc < 3) {
        //batch mode
        if (argc == 2) {
            int batch;
            if ((batch = open(argv[1], O_RDONLY, 0664)) < 0) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
            if ((dup2(batch, 0)) == -1) {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
        }
        int pathsize = 1;
        char **paths = (char **) calloc(1, sizeof(char *));
        paths[0] = "/bin";
        //interactive mode(no arg)
        while (1) {
            start:
            if (argc == 1) {
                printf("wish> ");
            }
            //token array allocation
            char *buffer = (char *) calloc(bufsize, sizeof(char));
            char *ogbuff = buffer;
            char **atoks = (char **) calloc(bufsize, sizeof(char *));

            //get line into buffer for tokenization
            getline(&buffer, &bufsize, stdin);
            //handle whitespace lines
            while (buffer[0] == ' ' || buffer[0] == '\t' || buffer[0] == '\n') {
                if (buffer[0] == '\n') {
                    free(ogbuff);
                    free(atoks);
                    goto start;
                } else { buffer = &buffer[1]; }
            }
            // Cmd token / Redirect Token / Arg token
            char *ctok;
            char *rtok;
            char *ptok;
            char *atok;
            //TOTAL: cmd count / arg count
            int c = 0;
            int a = 0;
            int cmdsize[MAXCMD][MAXCMD];
            memset(cmdsize, 0, MAXCMD * MAXCMD * sizeof(int));
            //>>>>>>>>>>>>>>>>>>>>>>>TOKENIZE INPUT>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            while ((ctok = strsep(&buffer, "&\n")) != NULL) {
                if (ctok[0] == '\0') { continue; }
                while (ctok[0] == ' ' || ctok[0] == '\t') {
                    ctok = &ctok[1];
                }
                // redirect(>) count per command
                int r = 0;
                int argvsize = 0;
                while ((rtok = strsep(&ctok, ">")) != NULL) {
                    if (rtok[0] == '\0') {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        free(atoks);
                        free(ogbuff);
                        goto start;
                    }
                    if (r == 1) {
                        atoks[a] = ">";
                        a++;
                    } else if (r > 1) {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        free(atoks);
                        free(ogbuff);
                        goto start;
                    }
                    int p = 0;
                    while ((ptok = strsep(&rtok, "|")) != NULL) {
                        if (p > 0) {
                            a++;
                            atoks[a] = "|";
                            a++;
                        }
                        while ((atok = strsep(&ptok, " \t\n")) != NULL) {
                            if (atok[0] != '\0') {
                                atoks[a] = atok;
                                argvsize++;
                                a++;
                            }
                        }
                        if (r == 0) {
                            cmdsize[c][p + 1] = argvsize;
                            argvsize = 0;
                            cmdsize[c][0]++;
                        }
                        p++;
                    }
                    a++;
                    r++;
                }
                c++;
            }
            //end of arg token
            atoks[a] = "$";
            //>>>>>>>>>>>>>>>>>>>>>>>END OF TOKENIZATION>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            //>>>>>>>>>>>>>>>>>>>>>>>Built in Commands>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            int cmdindex = 0;
            int tempin;
            int tempout;
            int temperror;
            int fd;
            int fdpipe[2] = {};
            tempin = dup(STDIN_FILENO);
            tempout = dup(STDOUT_FILENO);
            temperror = dup(STDERR_FILENO);
            for (int j = 0; j < c; j++) {
                //EXIT Command
                if (strcmp(atoks[cmdindex], "exit") == 0) {
                    if (cmdsize[j][1] == 1) { exit(0); }
                    else {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        free(atoks);
                        free(ogbuff);
                        goto start;
                    }
                }
                    //CD command
                else if (strcmp(atoks[cmdindex], "cd") == 0) {
                    //arg check
                    if (cmdsize[j][1] == 2) {
                        if ((chdir(atoks[cmdindex + 1])) == 0) {
                            free(atoks);
                            free(ogbuff);
                            goto start;
                        }
                    } else {
                        //error
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        free(atoks);
                        free(ogbuff);
                        goto start;
                    }
                }
                    //PATH command
                else if (strcmp(atoks[cmdindex], "path") == 0) {
                    for (int i = 1; i < pathsize; i++) {
                        free(paths[i]);
                    }
                    pathsize = cmdsize[j][1] - 1;
                    for (int i = 0; i < pathsize; i++) {
                        paths[i] = strdup(atoks[cmdindex + i + 1]);
                    }
                }
                    //>>>>>>>>>>>>>>>>>>>Fork Calls>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                else {
                    for (int p = 1; p < cmdsize[j][0] + 1; p++) {
                        //>>>>>>>>>>>>>>>>>>PIPES>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                        if (atoks[cmdindex + cmdsize[j][p] + 1][0] == '|') {
                            pipe(fdpipe);
                            //redirect stdout for cmd1
                            dup2(fdpipe[1], STDOUT_FILENO);
                            close(fdpipe[1]);
                        }
                            //>>>>>>>>>>>>>>>REDIRECTION>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                        else if (atoks[cmdindex + cmdsize[j][p] + 1][0] == '>') {
                            if ((fd = open(atoks[cmdindex + cmdsize[j][p] + 2],
                                           O_CREAT | O_RDWR | O_TRUNC, 0664)) < 0) {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                free(atoks);
                                free(ogbuff);
                                goto start;
                            }
                            if (atoks[cmdindex + cmdsize[j][p] + 3] != NULL) {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                close(fd);
                                free(atoks);
                                free(ogbuff);
                                goto start;
                            }
                            //redirect stdout to file
                            dup2(fd, STDOUT_FILENO);
                            //redirect stderr to file
                            dup2(fd, STDERR_FILENO);
                            close(fd);
                        } else {
                            dup2(tempout, STDOUT_FILENO);
                        }
                        //>>>>>>>>>>>>>>>FORK PATH CHECKING>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                        int cmdfound = 0;
                        for (int i = 0; i < pathsize; i++) {
                            char *path = strdup(paths[i]);
                            path = strcat(strcat(path, "/"), atoks[cmdindex]);
                            if (access(path, X_OK) == 0) {
                                cmdfound = 1;
                                int ret = fork();
                                if (ret == 0) {
                                    //childProcess
                                    if (atoks[cmdindex + cmdsize[j][p] + 1][0] == '|') {
                                        close(fdpipe[0]);
                                    }
                                    execv(path, &atoks[cmdindex]);
                                    //error
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                    _exit(1);
                                } else if (ret < 0) {
                                    //fork error
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                    exit(2);
                                } else {
                                    free(path);
                                    //parentProcess
                                    if (atoks[cmdindex + cmdsize[j][p] + 1][0] == '|') {
                                        dup2(fdpipe[0], STDIN_FILENO);
                                        close(fdpipe[0]);
                                        cmdindex += 2;
                                    } else if (atoks[cmdindex + cmdsize[j][p] + 1][0] == '>') {
                                        //restore stdout/stderr
                                        dup2(tempin, STDIN_FILENO);
                                        dup2(tempout, STDOUT_FILENO);
                                        dup2(temperror, STDERR_FILENO);
                                        cmdindex += 3;
                                    } else {
                                        dup2(tempin, STDIN_FILENO);
                                    }
                                }
                                break;
                            }
                            free(path);
                        }
                        if (!cmdfound) {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            free(atoks);
                            free(ogbuff);
                            goto start;
                        }
                        cmdindex += cmdsize[j][p];
                    }
                }
                cmdindex++;
            }
            free(atoks);
            free(ogbuff);
            close(tempin);
            close(tempout);
            close(temperror);
            //wait for all child process to finish
            while (wait(NULL) > 0) {}
            //breaks loop on eof
            if (feof(stdin)) {
                exit(0);
            }
        }
    } else {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
}
