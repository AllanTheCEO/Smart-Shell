#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h> 

//python include file
#include <Python.h>

/* "" means check the local directory */
#include "parser.h"

/* you'll need these includes later: */
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>	/* PATH_MAX */


int main(int argc, char **argv)
{   

    FILE *history = fopen("chat_history.log", "a+"); //Open file for LLM chat history

    //Helper function to clear the file
    void clear_file(FILE *path) {
        FILE *fp = fopen(path, "w");
        if (!fp) {
            perror("fopen");
            return;
        }
        fclose(fp);
    }

    
    bool interactive = isatty(STDIN_FILENO); /* see: man 3 isatty */
    FILE *fp = stdin;

    if (argc == 2) {
        interactive = false;
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
            exit(EXIT_FAILURE); /* see: man 3 exit */
        }
    }
    if (argc > 2) {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char line[1024], linebuf[1024];
    const int max_tokens = 32;
    char *tokens[max_tokens];
    
    /* loop:
     *   if interactive: print prompt
     *   read line, break if end of file
     *   tokenize it
     *   print it out <-- your logic goes here
     */

    //last status variable - need to initialize outside of while loop
    int last_status = 0;

    while (true) {

        signal(SIGINT, SIG_IGN); /* ignore SIGINT=^C */

        if (interactive) {
            /* print prompt. flush stdout, since normally the tty driver doesn't
             * do this until it sees '\n'
             */
            printf("$ ");
            fflush(stdout);
        }

        /* see: man 3 fgets (fgets returns NULL on end of file)
         */
        if (!fgets(line, sizeof(line), fp))
            break;

        /* read a line, tokenize it, and print it out
         */
        int n_tokens = parse(line, max_tokens, tokens, linebuf, sizeof(linebuf));

        /* new code goes here */

        //step 5 File Redirection
        char *in_file = NULL;
        char *out_file = NULL;

        //remove all instances of ">" and "<" and rebuild token array
        //Put input for last instance of "<" and ">" and put into infile or outfile
        for (int i = 0; i < n_tokens; i++) {
            if (strcmp(tokens[i], "<") == 0) {
                if (i + 1 < n_tokens) {
                    in_file = tokens[i + 1];
                }

                tokens[i] = NULL;
                tokens[1 + i] = NULL;
                i++;
            }
            else if (strcmp(tokens[i], ">") == 0) {
                if (i + 1 < n_tokens) {
                    out_file = tokens[i + 1];
                }
                tokens[i] = NULL;
                tokens[1 + i] = NULL;
                i++;
            }
        }

        //Find number of tokens that aren't null and set n_tokens to this number
        int index = 0;
        for (int i = 0; i < n_tokens; i++) {
            if (tokens[i] != NULL) {
                tokens[index] = tokens[i];
                index++;
            }
        }
        tokens[index] = NULL;
        n_tokens = index;

        //pipeline support
        //Count number of pipes, then add each command to an array of commands 
        int num_pipes = 0;
        for(int i = 0; i < n_tokens; i++) {
            if (strcmp(tokens[i], "|") == 0 && tokens[i] != NULL) {
                num_pipes++;
            }
        }
        
        char **commands = calloc(num_pipes + 1, sizeof(char*));
        if(commands == NULL) {
            perror("calloc");
            exit(1);
        }

        int temp = 0;
        int count = 0;
        int count_pipes = 0;
        //Tells program not to execute at all if there is an error
        //Loop through tokens and add each command to the commands array
        //Count the number of pipes
        
        //If there are no tokens, just behave normally
        if (num_pipes > 0) {
            for(int i = 0; i <= n_tokens; i++) {

                if(i == 0 && strcmp(tokens[i], "|") == 0) {
                    fprintf(stderr, "Error: Bad Pipe Syntax\n");
                    last_status = 1;
                    break;
                }
                    
                    
                if (i == n_tokens || strcmp(tokens[i], "|") == 0) {
    
                    
                    //Error for bad pipe syntax if if a token == null and it's not the last token
                    if (tokens[i + 1] == NULL && i != n_tokens) {  
                        fprintf(stderr, "Error: Bad Pipe Syntax\n");
                        break;
                        
                    } 
                    //Error if "||" or "| |" is found. 
                    if (i != n_tokens && strcmp(tokens[i + 1], "|") == 0) {
                        fprintf(stderr, "Error: Bad Pipe Syntax\n");
                        break;
                    } 
    
                    size_t len = 0;
                    count_pipes++;
                    for(int j = temp; j < i; j++) {
                        len += strlen(tokens[j]) + 1;
                    }
                    char *cmd = malloc(len);
                    if (cmd == NULL) {
                        perror("malloc");
                        exit(1);
                    }
                    cmd[0] = '\0';
                    for (int j = temp; j < i; j++) {
                        strcat(cmd, tokens[j]);
                        if(j < i - 1) {
                            strcat(cmd, " ");
                        }
                    }
    
                    commands[count] = cmd;
                    count++;
                    temp = i + 1;
    
                }
            }
        }
        

        //step 4 Special $? variable
        char qbuf[16];
        sprintf(qbuf, "%d", last_status);
        for (int i = 0; i < n_tokens; i++) {
            if (strcmp("$?", tokens[i]) == 0) {
                tokens[i] = qbuf;
            }
        }

        //step 2 cd, pwd, exit
        if (n_tokens == 0) {
            continue;
        }

        if (strcmp("cd", tokens[0]) == 0) {
            char *dir;
            if (n_tokens > 2) {
                fprintf(stderr, "cd: wrong number of arguments\n");
                last_status = 1;
                continue;
            }
            if (n_tokens == 1) {
                dir = getenv("HOME");
                if (dir == NULL) {
                    fprintf(stderr, "%s: directory not found\n");
                    last_status = 1;
                }
            }
            else {
                dir = tokens[1];
            }

            if (chdir(dir) != 0) {
                fprintf(stderr, "cd: %s\n", strerror(errno));
                last_status = 1;
            } else {
                last_status = 0;
            }
            continue;
        }

        else if (strcmp("pwd", tokens[0]) == 0) {
            if (n_tokens != 1) {
                fprintf(stderr, "%s: too many arguments\n");
            } else {
                char buffer[PATH_MAX];
                if (getcwd(buffer, PATH_MAX) == NULL) {
                    fprintf(stderr, "pwd: %s\n", strerror(errno));
                    last_status = 1;
                } else {
                    printf("%s\n", buffer);
                    last_status = 0;
                }
            }
            continue;
        }

        //?? sends a prompt to the LLM and prints a response
        else if (strcmp("??", tokens[0]) == 0) {

            pid_t pid = fork();
            if(pid < 0) {
                perror("fork");
                last_status = 1;
            } else if (pid == 0) {
                Py_Initialize(); //initialize python interpreter

                signal(SIGINT, SIG_DFL);
    
                PyRun_SimpleString(
                    "import sys\n"
                    "sys.path.insert(0, '.')\n"
                ); //import sys module
    
                char *text;
                if (n_tokens < 2) {
                    fprintf(stderr, "%s: need a prompt\n", argv[0]);
                    last_status = 1;
                } else {
    
                    PyObject *name = PyUnicode_FromString("chat");
                    PyObject *load_module = PyImport_Import(name);
                    
                    if (!load_module) {
                        PyErr_Print();
                        fprintf(stderr, "Error: could not import chat.py\n");
                    }
    
                    //Give current working directory to LLM
                    char cwd[PATH_MAX];
                    if(getcwd(cwd, sizeof(cwd)) == NULL ) {
                        perror("getcwd() error");
                    } else {
                        printf("Current working dir: %s\n", cwd);
                        PyObject *cwd_obj = PyUnicode_FromString(cwd);
                        PyObject *d = PyModule_GetDict(load_module);
                        PyDict_SetItemString(d, "CURRENT_DIR", cwd_obj);
                    }
    
                    //Read tokens into prompt string
                    char prompt[1024] = "";
                    for (int i = 1; i < n_tokens; i++) {
                        strncat(prompt, tokens[i], sizeof(prompt) - strlen(prompt) - 1);
    
                        if(i < n_tokens - 1) {
                            strncat(prompt, " ", sizeof(prompt) - strlen(prompt) - 1);
                        }
                    }
    
                    //print prompt to history file
                    fprintf(history, "%s\n", prompt); 
                    fflush(history);
    
                    //Call ask_bot function from chat.py
                    PyObject *func = PyObject_GetAttrString(load_module, "ask_bot");
    
                    if (!func || !PyCallable_Check(func)) { /* handle error */
                        PyErr_Print();
                        fprintf(stderr,"Error: ask_bot not found or not callable\n");
                    }
                    PyObject *args = PyTuple_Pack(1, PyUnicode_FromString(prompt));
                    PyObject *callfunc = PyObject_CallObject(func, args);
    
                    if (callfunc == NULL) { //handle error
                        PyErr_Print();
                        printf(stderr, "Error: LLM failed\n");
                    } else { //write to history file
                        const char *reply = PyUnicode_AsUTF8(callfunc);
                        fprintf(history, "%s\n", reply); 
                        fflush(history);
                    }
                    last_status = 0;
                }

            } else {
                int status;
                waitpid(pid, &status, 0);
                if (WIFSIGNALED(status)) {
                    last_status = 130;
                    fprintf(stderr, "\nLLM cancelled\n");
                }
                else if (WIFEXITED(status)) {
                    last_status = WEXITSTATUS(status);
                }
            }

            continue;
        }

        else if (strcmp("exit", tokens[0]) == 0) {
            if (n_tokens > 2) {
                fprintf(stderr, "exit: too many arguments\n", argv[0]);
                last_status = 1;
            } else if (n_tokens == 1) {
                clear_file("chat_history.log"); //close history file
                exit(0);
            } else if (n_tokens == 2) {
                clear_file("chat_history.log"); //close history file
                exit(atoi(tokens[1]));
            }
            continue;
        }

        
        //step 3 external commands
        //General fork for handling commands
        //Other forks are created if there are pipes
        pid_t pids = fork();

        if (pids < 0) {
            fprintf(stderr, "fork: %s\n", strerror(errno));
            last_status = 1;
        }
        else if (pids == 0) {
            signal(SIGINT, SIG_DFL);

            //Handle input redirection
            if (in_file) {
                int fd_in = open(in_file, O_RDONLY);
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }

            //Handle output redirection
            if (out_file) {
                int fd_out = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }

            //If there are pipes, create a pipe for each command and fork a child process for each command
            if (num_pipes + 1 > 1) { 
                int fds[num_pipes][2];

                
                for (int i = 0; i < num_pipes; i++) {
                    if (pipe(fds[i]) == -1) {
                        perror("pipe");
                        exit(EXIT_FAILURE);
                    }
                }

                // Debug statements to view the commands and num_pipes
                //fprintf(stderr, "Number of pipes: %d\n", num_pipes);
                //for (int i = 0; i < num_pipes + 1; i++) {
                    //fprintf(stderr, "Command %d: %s\n", i, commands[i]);
                //}

                for (int i = 0; i < num_pipes + 1; i++) {
                    pid_t pipeline_pid;

                    pipeline_pid = fork();

                    if (pipeline_pid < 0) {
                        perror("fork");
                        exit(EXIT_FAILURE);
                    }

                    if (pipeline_pid == 0) {

                        if (i > 0) {
                            dup2(fds[i - 1][0], STDIN_FILENO);
                            //close(fds[i - 1][0]);
                        }
                        if (i < num_pipes) {
                            dup2(fds[i][1], STDOUT_FILENO); 
                            //close(fds[i][1]);
                        } 
                        

                        for (int j = 0; j < num_pipes; j++) {
                            close(fds[j][0]);
                            close(fds[j][1]);
                        }

                        char *argv[max_tokens + 1];
                        char buf[1024];
                        parse(commands[i], max_tokens, argv, buf, sizeof(buf));

                        
                        execvp(argv[0], argv);
                        fprintf(stderr, "%s: %s\n", tokens[0], strerror(errno));
                        exit(EXIT_FAILURE);

                    }

                }

                for (int i = 0; i < num_pipes; i++) {
                    close(fds[i][0]);
                    close(fds[i][1]);
                }

                
                for (int i = 0; i < num_pipes + 1; i++) {
                    int status;
                    wait(&status);
                    if (WIFEXITED(status)) {
                        last_status = WEXITSTATUS(status);
                    }
                    else {                    
                        last_status = 1;
                    }
                    exit(last_status);
                }
  
            //If there are no pipes, just execute the command
            } else {
                execvp(tokens[0], tokens);
                fprintf(stderr, "%s: %s\n", tokens[0], strerror(errno));
                exit(EXIT_FAILURE);
            }

        }
        else {
            int status;
            do {
                waitpid(pids, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            } else {
                last_status = 1;
            }

        }




    }

    if (interactive)            /* make things pretty */
        printf("\n");           /* try deleting this and then quit with ^D */
}
