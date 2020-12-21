
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define MAXLEN 512
#define MAXTOKENS 10

char* read_line();
int execute(char**);
char** get_tokens(char*);

int inRedirectPresentAt(char** args);
int outRedirectPresentAt(char** args);
int ampersandPresentAt(char** args);

void handler(int sig) {
    int status;
    waitpid(-1, &status, WNOHANG); // non-blocking
}


int main()
{
    char *input;
    char **args;
    int status, count;
    char *prompt;

    signal(SIGCHLD, handler);

    do {
        // get the current working directory
        char* cwd = getcwd(NULL, 0);

        // allocate memory for prompt
        // +3 as 2 for the "> " and 1 for the terminating null character '\0'
        prompt = malloc( (strlen(cwd) + 3) * sizeof(char) );
        strcpy(prompt, cwd);
        strcat(prompt, "> ");

        free(cwd);

        printf("%s", prompt);

        // read the user input
        input = read_line();

        // separate the words
        args = get_tokens(input);

        // execute the command
        status = execute(args);

        free(args);
        free(input);
        free(prompt);
    } while (status);



    return 0;
}

char* read_line()
{
    char* line = (char*) malloc(MAXLEN + 1); // one extra for the terminating null character

    // read the input
    if (fgets(line, MAXLEN + 1, stdin) == NULL) {
        // returned NULL, but due to EOF
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        }

        // returned NULL due to failure
        else {
            perror("basic shell: fgets error\n");
            exit(EXIT_FAILURE);
        }
    }

    return line;
}

int execute(char** args)
{
    pid_t pid;
    int status;
    char* inFile;
    char* outFile;
    FILE* infp;
    FILE* outfp;
    int inRedirect;
    int outRedirect;
    int background;

    // if command is empty
    if (args[0] == NULL) {
        return 1;
    }

    // exit command
    if ( strcmp(args[0], "exit") == 0 ) {
        return 0;
    }

    // cd command
    if ( strcmp(args[0], "cd") == 0 ) {
        if (args[1] == NULL) {
            fprintf(stderr, "simple shell: expected argument to \"cd\"\n");
        } else {
            if (chdir(args[1]) != 0) {
              perror("simple shell");
            }
        }

        return 1;
    }

    // check for redirection
    inRedirect = inRedirectPresentAt(args);
    outRedirect = outRedirectPresentAt(args);

    // check for background process
    background = ampersandPresentAt(args);


    // create a child process
    pid = fork();

    // the child process gets 0 in return from fork()
    if (pid == 0) {
        // redirection
        if (inRedirect >= 0) {
            inFile = args[inRedirect + 1];
            args[inRedirect] = NULL;

            infp = freopen(inFile, "r", stdin);
        }

        if (outRedirect >= 0) {
            outFile = args[outRedirect + 1];
            args[outRedirect] = NULL;

            outfp = freopen(outFile, "w", stdout);
        }


        // background
        if (background >= 0) {
            args[background] = NULL;
        }

        // call execvp() to execute the user command
        // execvp() returns only on failure
        if (execvp(args[0], args) == -1) {
            perror("basic shell");
        }

        exit(EXIT_FAILURE);
    }
    else if (pid < 0) {
        // Error creating fork
        perror("basic shell");
    }
    // the parent process
    else {
        if (background < 0) {
            waitpid(pid, &status, 0);
        }
    }

    return 1;
}

char** get_tokens(char* line)
{
    int idx = 0;

    char delim[] = " \t\r\n";
    char **tokenList = malloc(MAXTOKENS * sizeof(char*));
    char *token;

    if (!tokenList) {
        perror("simple shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    // separate the tokens
    token = strtok(line, delim);
    while (token != NULL) {
        // iteratively add each token to tokenList
        tokenList[idx] = token;
        idx++;

        // if more than (MAXTOKENS - 1) tokens are found, ignore them
        if (idx >= MAXTOKENS - 1) {
            break;
        }

        // get the next token
        token = strtok(NULL, delim);
    }

    // the last token must be NULL as execvp() requires it to be NULL
    tokenList[idx] = NULL;
    return tokenList;
}

int inRedirectPresentAt(char** args)
{
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            return i;
        }
    }

    return -1;
}

int outRedirectPresentAt(char** args)
{
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            return i;
        }
    }

    return -1;
}

int ampersandPresentAt(char** args)
{
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "&") == 0) {
            return i;
        }
    }

    return -1;
}
