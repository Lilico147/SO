#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#define MAX_ARGS 64
#define MAX_CMD_LEN 256

/*-------------------------------------------------
             Trabalho de SO
       Aluno: Alexandre Cordeiro Arruda
       RGM 43551              3° ano CC
-------------------------------------------------*/

void executacomando(char *args[], int background) 
{
    pid_t pid = fork();
    
    if (pid < 0) 
    {
        perror("fork");
        exit(EXIT_FAILURE);
    } 
    
    else if (pid == 0) 
        
        {
        // Processo filho
        char* command = (char*) malloc(256);
        strcpy(command, "/bin/");
        strcat(command, args[0]);
        if (execve(command, args, __environ) == -1) 
        {
            perror("execve");
            exit(EXIT_FAILURE);
        }
        }

     else {
        // Processo pai
        if (!background) 
        {
            // Espera o processo filho terminar
            waitpid(pid, NULL, 0);
        }
          }
}

void arvore(int pid, int nivel) 
{
    DIR *dir;
    struct dirent *entry;
    char path[MAX_CMD_LEN];
    FILE *file;
    char buf[MAX_CMD_LEN];

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    if ((file = fopen(path, "r")) == NULL) 
    {
        perror("fopen");
        return;
    }

    fscanf(file, "%*d %*s %*c %*d"); // Lê e descarta PID, nome, estado e PPID
    int ppid;
    fscanf(file, "%d", &ppid); // Lê PPID
    fclose(file);

    for (int i = 0; i < nivel; ++i) 
    {
        printf("  ");
    }
    printf("%d\n", pid);

    if (ppid == pid) 
    {
        return; // final da Arvore 
    }

    snprintf(path, sizeof(path), "/proc/%d/task", pid);

    if ((dir = opendir(path)) == NULL) 
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
        {
            continue;
        }
        int child_pid = atoi(entry->d_name);
        arvore(child_pid, nivel + 1);
    }

    closedir(dir);
}

int main() 
{
    char input[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    int background = 0;

    while (1) {
        printf("Comando> ");
        fgets(input, MAX_CMD_LEN, stdin);

        // Remove o newline
        input[strcspn(input, "\n")] = '\0';

        // Verifica se o comando deve ser executado em segundo plano
        if (input[strlen(input) - 1] == '&') 
        {
            background = 1;
            input[strlen(input) - 1] = '\0'; // Remove o '&' do comando
        } 
        else 
        {
            background = 0;
        }

        // Parseia o comando
        char *token = strtok(input, " ");
        int i = 0;
        while (token != NULL) 
        {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        // imprime a árvore de processos
        if (strcmp(args[0], "tree") == 0 && args[1] != NULL) 
        {
            int pid = atoi(args[1]);
            printf("Árvore de Processos:\n");
            arvore(pid, 0);
        } 
        else 
        {
            executacomando(args, background);
        }
    }

    return 0;
}
