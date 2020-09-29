#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define STDIN 0
#define STDOUT 1
#define TRUE 1

int skip = 0;

/* cleans input string */
void clean(char *cmd){
    memset(cmd, 0, strlen(cmd));
}

/* cleans argument vector */
void clean_args(char **args){
    int i = 0;
    while(args[i] != NULL){
        memset(args[i], 0, strlen(args[i]));
        i++;
    }
}

/* removes spaces and special tokens from input string */
void wash_str(char *str){
    char *delim = " \n";
    if(str != NULL){
        int len = strlen(str);
        char *tmp = malloc(len*sizeof(char));
        int count = 0;
        int i;
        for(i = 0; i < len; i++){
            if(strchr(delim, str[i]) == NULL){
                tmp[count++] = str[i];
            }
        }
        strcpy(str, tmp);
    }
}

/* allocates space for argument vector */
void allocate_args(char **args){
    int i = 0;
    for(i = 0; i < 128; i++){
        args[i] = malloc(128*sizeof(char));
    }
}

/* tokenizes input string according to some delimiter */
int split(char *cmd, char **args, char *delim){
    char *tok = malloc(128*sizeof(char));
    int i = 0;

    tok = strtok(cmd, delim);
    args[i] = tok;
    i++;
    while(tok != NULL){
        tok = strtok(NULL, delim);
        args[i++] = tok;
    }
    return i-1;
}

/* parses input variable and takes out input/output files if present */
void get_in_out(char *cmd, char *infile, char *outfile){
    // check if outfile is present
    if(strchr(cmd, '>') != NULL){
        char **out_args = malloc(128*sizeof(char*));
        allocate_args(out_args);
        split(cmd, out_args, ">");
        strcpy(cmd, out_args[0]);                   // remove the outfile from the command string
        strcpy(outfile, out_args[1]);               // set outfile to the output file
        // check if the infile is within the outfile token
        if(strchr(outfile, '<') != NULL){
            char **in_args = malloc(128*sizeof(char*));
            allocate_args(in_args);
            split(outfile, in_args, ">");
            strcpy(outfile, in_args[0]);
            strcpy(infile, in_args[1]);
            return; // if program finds both in and out file no need to check further
        }
    } else{
        outfile = NULL;
    }
    // check if infile is present
    if(strchr(cmd, '<') != NULL){
        char **in_args = malloc(128*sizeof(char*));
        allocate_args(in_args);
        split(cmd, in_args, ">");
        strcpy(cmd, in_args[0]);
        strcpy(infile, in_args[1]);
    } else {
        infile = NULL;
    }
}

/* parses input variable and takes out & and sets background variable if needed */
void get_background(char *cmd, int *b){
    if(strchr(cmd, '&') != NULL){
        *b = 1;
        char **b_args = malloc(128*sizeof(char*));
        allocate_args(b_args);
        split(cmd, b_args, "&");
        strcpy(cmd, b_args[0]); 
    }
}

/* handles piping and forking of processes as determined by input command */
void run(char *cmd){
    char **args = malloc(128*sizeof(char*));
    char **pargs = malloc(128*sizeof(char*));
    char *delim = "|";
    int status;
    pid_t pid;                                      // stores current Proccess ID
    int sinfd = dup(STDIN);                         // keep track of STDIN file descriptor
    int soutfd = dup(STDOUT);                       // keep track of STDOUT file descriptor
    int proc_in;                                    // stores the process input
    int proc_out;                                   // stores the process output
    char *infile = malloc(128*sizeof(char));
    char *outfile = malloc(128*sizeof(char));
    int background = 0;
    
    // allocate memory
    allocate_args(args);

    // parse special characters and get in/out files if needed
    get_background(cmd, &background);
    get_in_out(cmd, infile, outfile);

    // remove spaces or special characters
    wash_str(infile);
    wash_str(outfile);

    // set input to process
    if(infile && infile[0]){
        if(access(infile, F_OK & R_OK)){
            proc_in = open(infile, O_RDONLY); // input = infile
        } else {
            perror("ERROR");
        }
    } else {
        proc_in = dup(sinfd); // input = STDIN
    }

    int len = split(cmd, pargs, delim);
    int i;
    for(i = 0; i < len; i++){
        // replace STDIN with process input
        dup2(proc_in, STDIN);
        close(proc_in);
        // tokenize the command
        split(pargs[i], args, " \n");

        // check if this is the last command
        if(i == len - 1){
            // if there is a specified outfile set proc_out = <filename> else proc_out = STDOUT
            if(outfile && outfile[0]){
                proc_out = open(outfile, O_RDWR | O_CREAT, 0666);
            } else {
                proc_out = dup(soutfd);
            }
        } else {
            // if not last command set out to write-end of pipe, set in to read end of pipe
            int pipefd[2];
            if(pipe(pipefd) < 0){
                perror("ERROR");
                return;
            }
            proc_out = pipefd[1];
            proc_in = pipefd[0];
        }

        // replace STDOUT with process output
        dup2(proc_out, STDOUT);
        close(proc_out);

        pid = fork();
        if(pid == 0){
            if(execvp(args[0], args) < 0){
                perror("ERROR");
                exit(1);
            }
        }
        clean_args(args);
    }

    /* restore STDIN and STDOUT */
    dup2(sinfd, STDIN);
    close(sinfd);
    dup2(soutfd, STDOUT);
    close(soutfd);

    if(pid > 0){
            
        if(!background){    
            // wait for child process to finish
            waitpid(-1, &status, 0);
        }
    }
    if(pid < 0){
        perror("ERROR");
    }
}

// wait for children to terminate and terminate zombies
void sigchld_handler(){
    while(TRUE){
        pid_t pid = waitpid(-1, NULL, WNOHANG);
        if(pid == 0 || pid == -1){
            return;
        }
    }
}

int main(int argc, char **argv){ 
    int execute = 1;
    char *cmd = malloc(512*sizeof(char));
    signal(SIGCHLD, sigchld_handler);
    while(execute){
        skip = 0;
        clean(cmd);
        if(argv[1] == NULL || strcmp(argv[1], "-n") != 0){
            write(STDOUT, "my_shell$ ", 10);
        }

        if(!read(STDIN, cmd, 512)){
            write(STDOUT, "\n", 1);
            execute = 0;
        }
        if(!skip){
            run(cmd);
        }
    }
    return 0;
}