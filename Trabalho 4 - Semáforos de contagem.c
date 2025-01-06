/*
Aluno: Alexandre Cordeiro Arruda  RGM:43551
		(T4 de SO)
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>

#define SEM_L1 "/sem_l1"
#define SEM_L2 "/sem_l2"
#define SEM_L3 "/sem_l3"

// Estrutura do nó para a lista encadeada
struct no_t {
    int dado;
    struct no_t *prox;
};

// Estrutura da lista encadeada
struct lista_enc_t {
    struct no_t *cabeca;
    pthread_mutex_t trava;
};

// Inicialização das listas encadeadas e seus mutexes
struct lista_enc_t lista1 = {NULL, PTHREAD_MUTEX_INITIALIZER};
struct lista_enc_t lista2 = {NULL, PTHREAD_MUTEX_INITIALIZER};
struct lista_enc_t lista3 = {NULL, PTHREAD_MUTEX_INITIALIZER};

// Semáforos para sincronização
sem_t *sem_l1;
sem_t *sem_l2;
sem_t *sem_l3;

// Função para inserir um nó no final da lista encadeada
void inserir_no_fim(struct lista_enc_t *lista, int valor) {
    struct no_t *novo = (struct no_t *)malloc(sizeof(struct no_t));
    novo->dado = valor;
    novo->prox = NULL;

    // Bloqueia a lista para inserção
    pthread_mutex_lock(&lista->trava);

    if (lista->cabeca == NULL) {
        lista->cabeca = novo;
    } else {
        struct no_t *temp = lista->cabeca;
        while (temp->prox != NULL) {
            temp = temp->prox;
        }
        temp->prox = novo;
    }

    // Desbloqueia a lista após inserção
    pthread_mutex_unlock(&lista->trava);
}

// Função para verificar se um número é primo
int eh_primo(int numero) {
    if (numero <= 1) return 0;
    if (numero <= 3) return 1;
    if (numero % 2 == 0 || numero % 3 == 0) return 0;
    for (int i = 5; i * i <= numero; i += 6) {
        if (numero % i == 0 || numero % (i + 2) == 0) return 0;
    }
    return 1;
}

// Função para imprimir a lista encadeada
void imprimir_lista(struct lista_enc_t *lista) {
    // Bloqueia a lista para leitura
    pthread_mutex_lock(&lista->trava);

    struct no_t *atual = lista->cabeca;
    while (atual != NULL) {
        printf("%d ", atual->dado);
        atual = atual->prox;
    }

    // Desbloqueia a lista após leitura
    pthread_mutex_unlock(&lista->trava);
    printf("\n");
}

// Thread para ler números de um arquivo e armazená-los na lista1
void *thread_leitura_arquivo(void *arg) {
    FILE *arq = fopen("in.txt", "r");
    if (!arq) {
        perror("Falha ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }

    int num;
    while (fscanf(arq, "%d", &num) != EOF) {
        inserir_no_fim(&lista1, num);
    }
    fclose(arq);

    // Sinaliza que a leitura foi concluída
    sem_post(sem_l1);
    return NULL;
}

// Thread para criar a lista2 a partir da lista1, excluindo pares maiores que 2
void *thread_proc_l1_para_l2(void *arg) {
    sem_wait(sem_l1); // Espera a leitura da lista1 ser concluída
    pthread_mutex_lock(&lista1.trava);

    struct no_t *atual = lista1.cabeca;
    while (atual != NULL) {
        if (!(atual->dado > 2 && atual->dado % 2 == 0)) {
            inserir_no_fim(&lista2, atual->dado);
        }
        atual = atual->prox;
    }

    pthread_mutex_unlock(&lista1.trava);
    sem_post(sem_l2); // Sinaliza que a criação da lista2 foi concluída
    return NULL;
}

// Thread para criar a lista3 a partir da lista2, excluindo números não primos
void *thread_proc_l2_para_l3(void *arg) {
    sem_wait(sem_l2); // Espera a criação da lista2 ser concluída
    pthread_mutex_lock(&lista2.trava);

    struct no_t *atual = lista2.cabeca;
    while (atual != NULL) {
        if (eh_primo(atual->dado)) {
            inserir_no_fim(&lista3, atual->dado);
        }
        atual = atual->prox;
    }

    pthread_mutex_unlock(&lista2.trava);
    sem_post(sem_l3); // Sinaliza que a criação da lista3 foi concluída
    return NULL;
}

// Thread para imprimir os números primos armazenados na lista3
void *thread_imprimir_l3(void *arg) {
    sem_wait(sem_l3); // Espera a criação da lista3 ser concluída
    imprimir_lista(&lista3); // Imprime os números primos
    return NULL;
}

int main() {
    pthread_t threads[4];

    // Criação dos semáforos
    sem_l1 = sem_open(SEM_L1, O_CREAT, 0644, 0);
    sem_l2 = sem_open(SEM_L2, O_CREAT, 0644, 0);
    sem_l3 = sem_open(SEM_L3, O_CREAT, 0644, 0);

    if (sem_l1 == SEM_FAILED || sem_l2 == SEM_FAILED || sem_l3 == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Criação das threads
    pthread_create(&threads[0], NULL, thread_leitura_arquivo, NULL);
    pthread_create(&threads[1], NULL, thread_proc_l1_para_l2, NULL);
    pthread_create(&threads[2], NULL, thread_proc_l2_para_l3, NULL);
    pthread_create(&threads[3], NULL, thread_imprimir_l3, NULL);

    // Espera a conclusão das threads
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    // Fechamento e desvinculação dos semáforos
    sem_close(sem_l1);
    sem_close(sem_l2);
    sem_close(sem_l3);
    sem_unlink(SEM_L1);
    sem_unlink(SEM_L2);
    sem_unlink(SEM_L3);

    return 0;
}

