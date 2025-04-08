#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <termios.h>
#include <signal.h>
#include "history.h"
#include "ps.h"
#include "BuiltInCommands.h"

#define PATH_MAX 256

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define MAX_INPUT 1024
#define MAXARGS 32

void exit_command();
void cd_command();
void history_command();
void ps_command();
void kill_command();
void bang_command();
void split_token_argument(char *input, char **token, char **argument);

struct command
{
    char *name;
    void (*function)();
    char *description;
};

int nBuilt = 6;
int runInBg = 0; // false
pid_t my_pid;

struct sigaction prije;

struct command builtInCommands[] = {
    {"exit", &exit_command, "Exit the shell"},
    {"cd", &cd_command, "Change the current directory"},
    {"history", &history_command, "Show the history of commands"},
    {"ps", &ps_command, "Show the current processes"},
    {"kill", &kill_command, "Kill a process"},
    {"!", &bang_command, "Repeat a command from history"}};
void printPrompt()
{
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    struct utsname detect;
    uname(&detect);
    printf(ANSI_COLOR_CYAN "%s#%s -> " ANSI_COLOR_RESET, detect.nodename, cwd);
}
void info_command(char *argument)
{
    if (!argument)
    {
        // print_logo();
        struct utsname detect;
        uname(&detect);
        printf("System name: %s\n", detect.sysname);
        printf("Node name: %s\n", detect.nodename);
        printf("Release: %s\n", detect.release);
        printf("Version: %s\n", detect.version);
        printf("Machine: %s\n", detect.machine);
        printf("Shell: ShiShell v1.0\n");
        return;
    }
    int i = 0;
    while (i < nBuilt)
    {
        if (strcmp(argument, builtInCommands[i].name) == 0)
        {
            printf("%s: %s\n", builtInCommands[i].name, builtInCommands[i].description);
            break;
        }
        i++;
    }
}

int run_command(char **args, int run_in_background)
{
    char *token = strdup(args[0]);
    if (args == NULL || args[0] == NULL)
        return -1;
    pid_t pid;
    if ((pid = fork()) == 0)
    {

        sigaction(SIGINT, &prije, NULL); // resetiraj signale
        setpgid(pid, pid);               // stvori novu grupu za ovaj proces
        if (!run_in_background)          // ako je u foregroundu
        {
            tcsetpgrp(STDIN_FILENO, getpgid(pid)); // dodijeli terminal
        }
        if (execvp(args[0], args))
        {
            fprintf(stderr, "Command not found\n");
            exit(1);
        }
        exit(0);
    }
    // printf("Nadodaj %d%s\n", pid, args[0]);
    add_process(pid, token); // dodaj proces u listu
    return pid;
}

int executeCommand(char *input)
{
    size_t vel_buf = 128;
    char buffer[vel_buf];

    char *line = strdup(input);

    if (!*input)
        return -1;
    if (input[0] == '!')
    {
        bang_command(strdup(input + 1), input);
    }

    add_history(input);

    //    split_token_argument(line, &token, &argument);
    //    if (!token)
    //        return;
    char *argv[MAXARGS];
    int argc = 0;
    argv[argc] = strtok(input, " \t\n");
    while (argv[argc] != NULL)
    {
        if (argv[argc][0] == '&')
        {
            // printf("Background process\n");
            runInBg = 1;
            argv[argc] = NULL;
            break;
        }

        argc++;
        argv[argc] = strtok(NULL, " \t\n");
    }

    int i = 0;

    while (i < nBuilt)
    {
        if (strcmp(argv[0], builtInCommands[i].name) == 0)
        {

            // if (fork() == 0)
            // {
            //     add_process(getpid(), argv[0]);
            builtInCommands[i].function(argv[1]);
            //     remove_process(getpid());
            //
            //     exit(0);
            // }
            // else
            // {
            //     wait(NULL);
            // }
            //
            return -1;
        }
        i++;
    }
    if (strcmp(argv[0], "info") == 0)
    {
        info_command(argv[1]);
        return -1;
    }

    return run_command(argv, runInBg);
}

void bang_command(char *argument, char *line)
{
    if (!argument)
    {
        fprintf(stderr, "Usage: !<number>\n");
        return;
    }

    int num = atoi(argument);
    if (num <= 0 || num > history_count)
    {
        fprintf(stderr, "Invalid history number\n");
        return;
    }
    strcpy(line, history[num - 1]);
    return;
}
struct sigaction prije;
pid_t pid;

void manage_sigint(int signal)
{

    // printf("pid: %d\n", pid);
    if (pid > 0)
    {
        kill(pid, SIGINT);
    }
    printf("\n");
    printPrompt();
    return;
}
void manage_signal(int signal)
{
    printf("\nSignal %d\n", signal);
}

void child_process_ended(int sig)
{
    pid_t pid_ended = waitpid(-1, NULL, WNOHANG); // ne čeka
    if (pid_ended > 0)
        if (kill(pid_ended, 0) == -1)
        { // možda je samo promijenio stanje ili je bas završio
            //  printf("\n[roditelj %d - SIGCHLD + waitpid] dijete %d zavrsilo s radom\n", (int)getpid(), pid_ended);
            remove_process(pid_ended);
            return;
        }
}

int main()
{
    my_pid = getpid();
    add_process(my_pid, "ShiShell");

    struct sigaction act;
    pid_t pid_ended;

    act.sa_handler = manage_signal;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = manage_sigint;
    sigaction(SIGINT, &act, &prije);
    act.sa_handler = child_process_ended;
    sigaction(SIGCHLD, &act, NULL);
    act.sa_handler = SIG_IGN;
    sigaction(SIGTTOU, &act, NULL); // zbog tcsetpgrp

    struct termios shell_term_settings;
    tcgetattr(STDIN_FILENO, &shell_term_settings);

    //print_logo();

    char input[MAX_INPUT];
    do
    {

        printPrompt();
        while (!fgets(input, sizeof(input), stdin))
        {
            // Add this check
            clearerr(stdin); // Reset error state
            continue;
            // Restart the input loop
        }

        input[strcspn(input, "\n")] = '\0';
        runInBg = 0;

        pid = executeCommand(input);

        if (!runInBg)
        {
            if (pid != -1)
            {
                do
                {
                    pid_ended = waitpid(pid, NULL, WNOHANG);
                    if (pid_ended > 0)
                    {
                        if (kill(pid, 0) == -1)
                        { // nema ga više? ili samo mijenja stanje

                            // vraćam terminal ljusci
                            tcsetpgrp(STDIN_FILENO, getpgid(0));
                            tcsetattr(STDIN_FILENO, 0, &shell_term_settings);
                        }
                        else
                        {
                            pid = (pid_t)0; // nije gotov
                        }
                    }
                } while (pid_ended <= 0);
                remove_process(pid);
            }
        }

    } while (strcmp(input, "exit") != 0);

    return 0;
}
