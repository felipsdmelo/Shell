#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

#define COLOR_GREEN "\033[0;32;32m"
#define COLOR_RED "\e[0;31m"
#define COLOR_YELLOW "\e[0;33m"
#define COLOR_BOLD_YELLOW "\033[1;33m"
#define COLOR_UNDERLINE_YELLOW "\e[4;33m"
#define COLOR_BACKGROUND_YELLOW "\e[43m"
#define COLOR_BLUE "\e[0;34m"
#define COLOR_BOLD_BLUE "\e[1;34m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_RESET "\x1b[0m"
#define CLEAR "\e[2J\e[H"

#define SHELL_BUFSIZE 1024
#define BUFSIZE_TOK 64
#define TOK_DELIM " \t\r\n\a"

int cd(char **args);
int help(char **args);
int shellExit(char **args);

char *builtin_str[] = { "cd", "help", "exit", "jobs"};

typedef struct jobs {
    int posicao;
    char status[20];
    char nome[100];
    pid_t processo;
} jobs;

static jmp_buf env;
pid_t pid;
jobs listaJobs[200];
int qtdJobs = 0;

int imprimeJobs(); // declaraçao da funçao imprimeJobs
int (*builtin_func[]) (char **) = {&cd, &help, &shellExit, &imprimeJobs};

void killJobs(pid_t id)
{
    for (int i = 0 ; i < qtdJobs ; i++)
    {
        if (listaJobs[i].processo == id)
        {
            kill(id, SIGKILL);
            strcpy(listaJobs[i].status, "Done");
        }
    }
}

void sigHandler(int signum)
{
    if (signum == SIGINT) // Ctrl + C
    {
        if (pid != 0)
        {
            killJobs(pid);
            longjmp(env, 42);
        }
    }
    else if (signum == SIGTSTP) // Ctrl + Z
    {
        if (pid != 0)
        {
            killJobs(pid);
            longjmp(env, 42);
        }
    }
    fflush(stdout);
}

int qtdBuiltIns() {
    return sizeof(builtin_str) / sizeof(char *);
}

int cd(char **args)
{
    if (args[1] == NULL)
        fprintf(stderr, COLOR_RED "MyShell: Argumento esperado para \"cd\"\n" COLOR_RESET);
    else
    {
        if (chdir(args[1]) != 0)
            perror(COLOR_RED "MyShell>" COLOR_RESET);
    }

    return 1;
}

int help(char **args)
{
    printf(COLOR_BOLD_BLUE "help:\n" COLOR_RESET);
    for (int i = 0 ; i < qtdBuiltIns() ; i++)
        printf(COLOR_CYAN "\t%s\n" COLOR_RESET, builtin_str[i]);

    return 1;
}

int shellExit(char **args) {
    exit(0);
}

void registraJob(char **args, pid_t id)
{
    jobs job;
    job.processo = id;
    strcpy(job.nome, args[0]);
    strcpy(job.status, "Running\t");
    job.posicao = qtdJobs + 1;
    listaJobs[qtdJobs] = job;
    qtdJobs++;
}

int imprimeJobs()
{
    for (int i = 0 ; i < qtdJobs ; i++)
        printf(COLOR_CYAN "[%d]  Pid: %d   %s\t%s\n" COLOR_RESET, 
            listaJobs[i].posicao, listaJobs[i].processo, listaJobs[i].status, listaJobs[i].nome);

    return 1;
}

int launch(char **args)
{
    int status;
    pid_t pide;
    pide = fork();
    if (pide == 0) // processo filho
    {
        if (execvp(args[0], args) == -1)
            perror(COLOR_RED "MyShell>" COLOR_RESET);
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
        perror(COLOR_RED "MyShell>" COLOR_RESET);
    else
    {
        do {
            registraJob(args, pide);
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;    
}

int execute(char **args)
{
    if (args[0] == NULL)
        return 1;

    for (int i = 0 ; i < qtdBuiltIns() ; i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
            return (*builtin_func[i])(args);
    }

    return launch(args);
}

char *lerLinha()
{
    int bufsize = SHELL_BUFSIZE;
    int posicao = 0;
    char *buffer = (char *) malloc(bufsize * sizeof(char));
    int c;

    if (!buffer)
    {
        fprintf(stderr, COLOR_RED "MyShell: Erro de alocacao" COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        c = getchar();
        if (c == EOF) {
            exit(EXIT_SUCCESS);
        }
        else if (c == '\n')
        {
            buffer[posicao] = '\0';
            return buffer;
        }
        else {
            buffer[posicao] = c;
        }
        posicao++;

        if (posicao >= bufsize)
        {
            bufsize += SHELL_BUFSIZE;
            buffer = realloc(buffer, bufsize);

            if (!buffer)
            {
                fprintf(stderr, COLOR_RED "MyShell: Erro de alocacao" COLOR_RESET);
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **splitLinha(char *linha)
{
    int bufsize = BUFSIZE_TOK;
    int posicao = 0;
    char **tokens = (char **) malloc(bufsize * sizeof(char *));
    char *token, **tokensBackup;

    if (!tokens)
    {
        fprintf(stderr, COLOR_RED "MyShell: Erro de alocacao" COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    token = strtok(linha, TOK_DELIM);
    while (token != NULL)
    {
        tokens[posicao] = token;
        posicao++;

        if (posicao >= bufsize)
        {
            bufsize += BUFSIZE_TOK;
            tokensBackup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                free(tokensBackup);
                fprintf(stderr, COLOR_RED "MyShell: Erro de alocacao" COLOR_RESET);
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, TOK_DELIM);
    }

    tokens[posicao] = NULL;
    return tokens;
}

void loop()
{
    char *linha;
    char **args;
    int status;
    char cwd[200]; // caminho do diretorio
    signal(SIGINT, sigHandler);
    signal(SIGTSTP, sigHandler);

    do
    {
        if (setjmp(env) == 42)
        {
            printf("\n");
            continue;
        }
        //printf(COLOR_BACKGROUND_YELLOW "MyShell" COLOR_RESET);
        //printf(COLOR_UNDERLINE_YELLOW "MyShell" COLOR_RESET);
        printf(COLOR_UNDERLINE_YELLOW "MyShell" COLOR_YELLOW ": %s> " COLOR_RESET, 
            getcwd(cwd, sizeof(cwd)));
        fflush(stdout);
        linha = lerLinha();
        args = splitLinha(linha);
        // registraJob(args);
        status = execute(args);

        free(linha); free(args);
    } while (status);
}

int main()
{
    //printf(CLEAR);
    signal(SIGINT, sigHandler);
    signal(SIGTSTP, sigHandler);
    loop();
    return 0;
}
