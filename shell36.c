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

    Py_Initialize(); //initialize python interpreter

    
    PyRun_SimpleString(
        "import sys\n"
        "sys.path.insert(0, '.')\n"
    ); //import sys module
    

    signal(SIGINT, SIG_IGN); /* ignore SIGINT=^C */

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
        //Set
        int index = 0;
        for (int i = 0; i < n_tokens; i++) {
            if (tokens[i] != NULL) {
                tokens[index] = tokens[i];
                index++;
            }
        }
        tokens[index] = NULL;
        n_tokens = index;


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
            char *text;
            if (n_tokens < 2) {
                fprintf(stderr, "%s: need a prompt\n", argv[0]);
                last_status = 1;
            } else {

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
                PyObject *name = PyUnicode_FromString("chat");
                PyObject *load_module = PyImport_Import(name);
                
                if (!load_module) {
                    PyErr_Print();
                    fprintf(stderr, "Error: could not import chat.py\n");
                }

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

            execvp(tokens[0], tokens);
            fprintf(stderr, "%s: %s\n", tokens[0], strerror(errno));
            exit(EXIT_FAILURE);
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
