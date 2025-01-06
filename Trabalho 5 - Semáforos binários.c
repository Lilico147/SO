/*
Aluno: Alexandre Cordeiro Arruda  RGM:43551
		(T5 de SO)
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

// Estrutura do node da lista
typedef struct Node {
    int value;
    struct Node* next;
    sem_t node_sem;
} Node;

// Cabeça da lista
Node* list_head = NULL;

// Função para criar um novo node
Node* create_node(int value) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->value = value;
    new_node->next = NULL;
    sem_init(&new_node->node_sem, 0, 1); // Inicializa o semaforo do node com valor = 1
    return new_node;
}

// Funcao para inserir um node no final da lista
void insert_end(int value) {
    Node* new_node = create_node(value);

    if (list_head == NULL) {
        list_head = new_node;
    } else {
        Node* temp = list_head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_node;
    }
}

// Funçao para verificar se um numero é primo
bool is_prime(int num) {
    if (num <= 1) return false;
    if (num <= 3) return true;
    if (num % 2 == 0 || num % 3 == 0) return false;
    for (int i = 5; i * i <= num; i += 6) {
        if (num % i == 0 || num % (i + 2) == 0) return false;
    }
    return true;
}

// Funçao para remover numeros pares maiores que 2
void* remove_even_greater_than_two(void* arg) {
    while (true) {
        Node** current = &list_head;

        while (*current) {
            Node* entry = *current;
            sem_wait(&entry->node_sem);

            if (entry->value % 2 == 0 && entry->value > 2) {
                *current = entry->next;
                sem_post(&entry->node_sem);
                sem_destroy(&entry->node_sem);
                free(entry);
            } else {
                current = &entry->next;
                sem_post(&entry->node_sem);
            }
        }
        pthread_exit(NULL); // Sai do loop após conclusao
    }
}

// Funçao para remover numeros não primos
void* remove_non_primes(void* arg) {
    while (true) {
        Node** current = &list_head;

        while (*current) {
            Node* entry = *current;
            sem_wait(&entry->node_sem);

            if (!is_prime(entry->value)) {
                *current = entry->next;
                sem_post(&entry->node_sem);
                sem_destroy(&entry->node_sem);
                free(entry);
            } else {
                current = &entry->next;
                sem_post(&entry->node_sem);
            }
        }
        pthread_exit(NULL); // Sai do loop apos conclusão
    }
}

// Funçao para imprimir numeros primos
void* print_primes(void* arg) {
    while (true) {
        Node* current = list_head;

        while (current) {
            sem_wait(&current->node_sem);
            if (is_prime(current->value)) {
                printf("%d\n", current->value);
            }
            sem_post(&current->node_sem);
            current = current->next;
        }
        pthread_exit(NULL); // Sai do loop após conclusão
    }
}

int main() {
    // Criaçao das threads
    pthread_t thread1, thread2, thread3;

    // Le os numeros do arquivo e insere na lista
    FILE* file = fopen("in.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    int number;
    while (fscanf(file, "%d", &number) == 1) {
        insert_end(number);
    }
    fclose(file);

    // Cria as threads
    pthread_create(&thread1, NULL, remove_even_greater_than_two, NULL);
    pthread_create(&thread2, NULL, remove_non_primes, NULL);
    pthread_create(&thread3, NULL, print_primes, NULL);

    // Espera as threads terminarem
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

    return EXIT_SUCCESS;
}

