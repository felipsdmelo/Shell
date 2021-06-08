#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

#define COLOR_RED "\e[0;31m"
#define COLOR_YELLOW "\e[0;33m"
#define COLOR_BOLD_YELLOW "\033[1;33m"
#define COLOR_UNDERLINE_YELLOW "\e[4;33m"
#define COLOR_BOLD_BLUE "\e[1;34m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_RESET "\x1b[0m"
#define CLEAR "\e[2J\e[H"

#define BUFFER_SIZE 2048
#define BUFFER_SIZE_TOKEN 128
#define LIMITADORES " \t\r\n\a"

// declaracoes de funcoes built-in
int cd(char **args);
int help(char **args);
int shellExit(char **args);
int bg(char **args);

int imprimeJob(); // declaracao da funçao imprimeJob
int (*builtin_func[]) (char **) = {&cd, &help, &shellExit, &imprimeJob, &bg};
char *builtin_str[] = { "cd", "help", "exit", "jobs", "bg"};

typedef struct jobs {
    int posicao;
    char status[20];
    char nome[100];
    pid_t processo;
} jobs;

static jmp_buf buf;
pid_t pid;
jobs listaJobs[200];
int qtdJobs = 0;
int array[20];


void killJob(pid_t id)
{
    for (int i = 0 ; i < qtdJobs ; i++)
    {
        if (listaJobs[i].processo == id)
        {
            kill(id, SIGKILL);
            strcpy(listaJobs[i].status, "Killed  ");
        }
    }
}


void suspendJob(pid_t id)
{
    printf("\n");
    for (int i = 0 ; i < qtdJobs ; i++)
    {
        if (listaJobs[i].processo == id)
        {
            kill(id, SIGTSTP);
            strcpy(listaJobs[i].status, "Stopped");
        }
    }
}


void sigHandler(int signum)
{
    if (signum == SIGINT) // Ctrl + C
    {
        if (pid != 0)
            killJob(pid);
    }
    else if (signum == SIGTSTP) // Ctrl + Z
    {
        if (pid != 0)
        {
            kill(pid, SIGTSTP);
            longjmp(buf, 42); // restaura o processo para o estado que estava quando setjmp foi chamada
        }
    }
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
        if (chdir(args[1]) != 0)    // muda o diretório para o especificado em args[1]
            perror(COLOR_RED "MyShell>" COLOR_RESET);
    }

    return 1;
}


int bg(char **args)
{
    int percent = '%';
    if (fork() == 0)
    {
        int aux;
        if (strcmp(args[1], percent) == 0)
            aux = array[strtol(args + 1, NULL, 10)];
        else
            aux = atoi(args);

        kill(aux, SIGCONT);
    }
    return 1;
}


int help(char **args)
{
    printf(COLOR_BOLD_BLUE "help:\n" COLOR_RESET);
    for (int i = 0 ; i < qtdBuiltIns() ; i++)   // percorre a lista de comandos built-in
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
    job.posicao = qtdJobs + 1;  // salva a ordem em que o job foi executado
    listaJobs[qtdJobs] = job;
    qtdJobs++;
}


int imprimeJob()
{
    for (int i = 0 ; i < qtdJobs ; i++)
        printf(COLOR_BOLD_BLUE "[%d]  Pid: %d\t%s\t%s\n" COLOR_RESET, 
            listaJobs[i].posicao, listaJobs[i].processo, listaJobs[i].status, listaJobs[i].nome);

    return 1;
}


int naoBuiltInCommand(char **args)
{
    int status;
    pid = fork();
    if (pid == 0) // processo filho
    {
        if (execvp(args[0], args) == -1)
            perror(COLOR_RED "MyShell>" COLOR_RESET);
        exit(1);
    }
    else if (pid < 0)
        perror(COLOR_RED "MyShell>" COLOR_RESET);
    else
    {
        do {
            registraJob(args, pid);
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;    
}


int builtInCommand(char **args)
{
    if (args[0] == NULL)
        return 1;

    for (int i = 0 ; i < qtdBuiltIns() ; i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
            return (*builtin_func[i])(args);
    }

    return naoBuiltInCommand(args);
}


char *lerLinha()
{
    int tamanhoBuf = BUFFER_SIZE;
    int posicao = 0;
    char *buffer = (char *) malloc(tamanhoBuf * sizeof(char));
    int ch;

    if (!buffer)
    {
        fprintf(stderr, COLOR_RED "MyShell: Erro de alocacao" COLOR_RESET);
        exit(1);
    }

    while (1)
    {
        ch = getchar();
        if (ch == EOF) {
            exit(0);
        }
        else if (ch == '\n')
        {
            buffer[posicao] = '\0';
            return buffer;
        }
        else {
            buffer[posicao] = ch;
        }
        posicao++;

        if (posicao >= tamanhoBuf)
        {
            tamanhoBuf += BUFFER_SIZE;
            buffer = realloc(buffer, tamanhoBuf);

            if (!buffer)
            {
                fprintf(stderr, COLOR_RED "MyShell: Erro de alocacao" COLOR_RESET);
                exit(1);
            }
        }
    }
}


char **splitLinha(char *linha)
{
    int tamanhoBuf = BUFFER_SIZE_TOKEN;
    int posicao = 0;
    char **buf = (char **) malloc(tamanhoBuf * sizeof(char *));
    char *token, **bufBackup;

    if (!buf)
    {
        fprintf(stderr, COLOR_RED "MyShell: Erro de alocacao" COLOR_RESET);
        exit(1);
    }

    token = strtok(linha, LIMITADORES);
    while (token != NULL)
    {
        buf[posicao] = token;
        posicao++;

        if (posicao >= tamanhoBuf)
        {
            tamanhoBuf += BUFFER_SIZE_TOKEN;
            bufBackup = buf;
            buf = realloc(buf, tamanhoBuf * sizeof(char *));
            if (!buf)
            {
                free(bufBackup);
                fprintf(stderr, COLOR_RED "MyShell: Erro de alocacao" COLOR_RESET);
                exit(1);
            }
        }
        token = strtok(NULL, LIMITADORES);
    }

    buf[posicao] = NULL;
    return buf;
}


void inicializa()
{
    char *linha;
    char **args;
    int status;
    char cwd[256]; // caminho do diretorio corrente
    signal(SIGINT, sigHandler); // lida com os sinais antes
    signal(SIGTSTP, sigHandler);

    do
    {
        // ponto de volta do salto
        if (setjmp(buf) == 42)
        {
            printf("\nNEYYY NEY NEY ");
            continue;
        }
        printf(COLOR_UNDERLINE_YELLOW "MyShell" COLOR_YELLOW ": %s$ " COLOR_RESET, 
            getcwd(cwd, sizeof(cwd)));
        fflush(stdout);
        linha = lerLinha();
        args = splitLinha(linha);
        status = builtInCommand(args);

        free(linha); free(args);
    } while (status);
}


int main()
{
    //printf(CLEAR);
    inicializa();
    return 0;
}
