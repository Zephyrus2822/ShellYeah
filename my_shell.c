#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>

#define MAXCOM 1000 // max number of letters to be supported 
#define MAXLIST 100 // max number of commands to be supported 

#define clear() printf("\033[H\033[J")

void init_shell()
{
    clear();
    char* username = getenv("USER");
    printf("USER is: @%s\n", username);
    sleep(1);
    clear();
}

int take_input(char* str)
{
    char* buf;

    buf = readline(">>> ");
    if (strlen(buf) != 0) {
        add_history(buf);
        strcpy(str, buf);
        return 0;
    } else {
        return 1;
    }
}

void print_dir()
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("\nDir: %s", cwd);
    } else {
        perror("getcwd() error");
    }
}

void exec_arg(char** parsed)
{
    pid_t pid = fork();

    if (pid == -1) {
        printf("Failed forking child...\n");
        return;
    } else if (pid == 0) {
        if (execvp(parsed[0], parsed) < 0) {
            printf("Could not execute command...\n");
        }
        exit(0);
    } else {
        wait(NULL);
        return;
    }
}

void exec_arg_piped(char** parsed, char** parsedpipe)
{
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) < 0) {
        printf("Pipe could not be initialized\n");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("Could not fork\n");
        return;
    }

    if (p1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(parsed[0], parsed) < 0) {
            printf("Could not execute command 1...\n");
            exit(0);
        }
    } else {
        p2 = fork();

        if (p2 < 0) {
            printf("Could not fork\n");
            return;
        }

        if (p2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(parsedpipe[0], parsedpipe) < 0) {
                printf("Could not execute command 2...\n");
                exit(0);
            }
        } else {
            close(pipefd[0]);
            close(pipefd[1]);
            wait(NULL);
            wait(NULL);
        }
    }
}

void open_help()
{
    puts("\nMy first custom command shell:"
         "\nList of Commands supported:"
         "\n>cd"
         "\n>ls"
         "\n>exit"
         "\n>all other general commands available in UNIX shell"
         "\n>pipe handling"
         "\n>improper space handling");
    return;
}

int cmd_handler(char** parsed)
{
    int n = 4, i, swt = 0;
    const char* list[n];
    char* username;

    list[0] = "exit";
    list[1] = "cd";
    list[2] = "help";
    list[3] = "hello";

    for (i = 0; i < n; i++) {
        if (strcmp(parsed[0], list[i]) == 0) {
            swt = i + 1;
            break;
        }
    }

    switch (swt) {
        case 1:
            printf("\nGoodbye\n");
            exit(0);
        case 2:
            chdir(parsed[1]);
            return 1;
        case 3:
            open_help();
            return 1;
        case 4:
            username = getenv("USER");
            printf("%s\nPlease give yourself a respectable name?\n", username);
            return 1;
        default:
            printf("\nInvalid Command\n");
            break;
    }
    return 0;
}

int parse_piped(char* str, char** strpiped)
{
    int i;
    for (i = 0; i < 2; i++) {
        strpiped[i] = strsep(&str, "|");
        if (strpiped[i] == NULL)
            break;
    }

    if (strpiped[1] == NULL)
        return 0;
    else {
        return 1;
    }
}

void parse_space(char* str, char** parsed)
{
    int i;

    for (i = 0; i < MAXLIST; i++) {
        parsed[i] = strsep(&str, " ");

        if (parsed[i] == NULL)
            break;
        if (strlen(parsed[i]) == 0)
            i--;
    }
}

int process_string(char* str, char** parsed, char** parsedpipe)
{
    char* strpiped[2];
    int piped = 0;

    piped = parse_piped(str, strpiped);

    if (piped) {
        parse_space(strpiped[0], parsed);
        parse_space(strpiped[1], parsedpipe);
    } else {
        parse_space(str, parsed);
    }

    if (cmd_handler(parsed))
        return 0;
    else
        return 1 + piped;
}

int main()
{
    char inputString[MAXCOM], *parsedArgs[MAXLIST];
    char* parsedArgsPiped[MAXLIST];
    int execFlag = 0;
    init_shell();

    while (1) {
        print_dir();
        if (take_input(inputString))
            continue;
        execFlag = process_string(inputString, parsedArgs, parsedArgsPiped);
        if (execFlag == 1)
            exec_arg(parsedArgs);
        if (execFlag == 2)
            exec_arg_piped(parsedArgs, parsedArgsPiped);
    }
    return 0;
}
