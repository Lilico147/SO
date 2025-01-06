#include <stdio.h>
#include <stdlib.h>

#define MAX_FRAMES 100
#define MAX_ADDR 65536

// Algoritmos de substituição
int fifo(int *paginas, int tam_pag, int num_frames, int num_acessos, int *falhas, FILE *out);
int opt(int *paginas, int tam_pag, int num_frames, int num_acessos, int *falhas, FILE *out);
int lru(int *paginas, int tam_pag, int num_frames, int num_acessos, int *falhas, FILE *out);

// Leitura dos endereços do arquivo
int *ler_enderecos(const char *arquivo, int *num_acessos);

// Calcular número da página a partir do endereço
int obter_num_pagina(int endereco, int tam_pag);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: %s <tam_pagina> <tam_memoria> <arquivo_enderecos>\n", argv[0]);
        return 1;
    }

    int tam_pag = atoi(argv[1]);
    int tam_mem = atoi(argv[2]);
    const char *arquivo = argv[3];
    
    int num_frames = tam_mem / tam_pag;
    int num_acessos;
    
    int *enderecos = ler_enderecos(arquivo, &num_acessos);
    if (!enderecos) {
        printf("Erro ao ler o arquivo de endereços.\n");
        return 1;
    }

    // Variáveis de falhas para cada algoritmo
    int falhas_fifo = 0, falhas_opt = 0, falhas_lru = 0;

    FILE *out = fopen("erros.out", "w");
    if (!out) {
        printf("Erro ao abrir o arquivo erros.out\n");
        return 1;
    }

    // Executa os algoritmos e exibe os resultados
    fifo(enderecos, tam_pag, num_frames, num_acessos, &falhas_fifo, out);
    opt(enderecos, tam_pag, num_frames, num_acessos, &falhas_opt, out);
    lru(enderecos, tam_pag, num_frames, num_acessos, &falhas_lru, out);

    // Calcula as porcentagens de falha
    double perc_fifo = (double)falhas_fifo / num_acessos * 100;
    double perc_opt = (double)falhas_opt / num_acessos * 100;
    double perc_lru = (double)falhas_lru / num_acessos * 100;

    // Exibe o resumo final
    printf("FIFO: %d erros (%.2f%%)\n", falhas_fifo, perc_fifo);
    printf("OPT: %d erros (%.2f%%)\n", falhas_opt, perc_opt);
    printf("LRU: %d erros (%.2f%%)\n", falhas_lru, perc_lru);

    fprintf(out, "FIFO: %d erros (%.2f%%)\n", falhas_fifo, perc_fifo);
    fprintf(out, "OPT: %d erros (%.2f%%)\n", falhas_opt, perc_opt);
    fprintf(out, "LRU: %d erros (%.2f%%)\n", falhas_lru, perc_lru);

    fclose(out);
    free(enderecos);
    return 0;
}

int *ler_enderecos(const char *arquivo, int *num_acessos) {
    FILE *arq = fopen(arquivo, "r");
    if (!arq) return NULL;

    int *enderecos = malloc(MAX_ADDR * sizeof(int));
    int endereco, count = 0;

    while (fscanf(arq, "%d", &endereco) != EOF) {
        enderecos[count++] = endereco;
    }

    *num_acessos = count;
    fclose(arq);
    return enderecos;
}

int obter_num_pagina(int endereco, int tam_pag) {
    return endereco / tam_pag;
}

int pagina_na_memoria(int *frames, int num_frames, int pagina) {
    for (int i = 0; i < num_frames; i++) {
        if (frames[i] == pagina) return 1;
    }
    return 0;
}

int fifo(int *paginas, int tam_pag, int num_frames, int num_acessos, int *falhas, FILE *out) {
    int frames[MAX_FRAMES], idx = 0;

    for (int i = 0; i < num_frames; i++) frames[i] = -1;

    for (int i = 0; i < num_acessos; i++) {
        int pagina = obter_num_pagina(paginas[i], tam_pag);

        if (!pagina_na_memoria(frames, num_frames, pagina)) {
            frames[idx] = pagina;
            idx = (idx + 1) % num_frames;
            (*falhas)++;
            fprintf(out, "FIFO Page fault: endereço %d, página %d\n", paginas[i], pagina);
            printf("FIFO Page fault: endereço %d, página %d\n", paginas[i], pagina);
        }
    }

    return *falhas;
}

int opt(int *paginas, int tam_pag, int num_frames, int num_acessos, int *falhas, FILE *out) {
    int frames[MAX_FRAMES];

    for (int i = 0; i < num_frames; i++) frames[i] = -1;

    for (int i = 0; i < num_acessos; i++) {
        int pagina = obter_num_pagina(paginas[i], tam_pag);

        if (!pagina_na_memoria(frames, num_frames, pagina)) {
            if (i < num_frames) {
                frames[i] = pagina;
            } else {
                int mais_distante = -1, idx_subst = -1;

                for (int j = 0; j < num_frames; j++) {
                    int prox_uso = -1;
                    for (int k = i + 1; k < num_acessos; k++) {
                        if (obter_num_pagina(paginas[k], tam_pag) == frames[j]) {
                            prox_uso = k;
                            break;
                        }
                    }
                    if (prox_uso == -1) {
                        idx_subst = j;
                        break;
                    } else if (prox_uso > mais_distante) {
                        mais_distante = prox_uso;
                        idx_subst = j;
                    }
                }
                frames[idx_subst] = pagina;
            }
            (*falhas)++;
            fprintf(out, "OPT Page fault: endereço %d, página %d\n", paginas[i], pagina);
            printf("OPT Page fault: endereço %d, página %d\n", paginas[i], pagina);
        }
    }

    return *falhas;
}

int lru(int *paginas, int tam_pag, int num_frames, int num_acessos, int *falhas, FILE *out) {
    int frames[MAX_FRAMES], ult_uso[MAX_FRAMES];

    for (int i = 0; i < num_frames; i++) {
        frames[i] = -1;
        ult_uso[i] = -1;
    }

    for (int i = 0; i < num_acessos; i++) {
        int pagina = obter_num_pagina(paginas[i], tam_pag);

        if (!pagina_na_memoria(frames, num_frames, pagina)) {
            int menos_usado = i, idx_subst = -1;

            for (int j = 0; j < num_frames; j++) {
                if (frames[j] == -1) {
                    idx_subst = j;
                    break;
                } else if (ult_uso[j] < menos_usado) {
                    menos_usado = ult_uso[j];
                    idx_subst = j;
                }
            }
            frames[idx_subst] = pagina;
            ult_uso[idx_subst] = i;
            (*falhas)++;
            fprintf(out, "LRU Page fault: endereço %d, página %d\n", paginas[i], pagina);
            printf("LRU Page fault: endereço %d, página %d\n", paginas[i], pagina);
        } else {
            for (int j = 0; j < num_frames; j++) {
                if (frames[j] == pagina) {
                    ult_uso[j] = i;
                    break;
                }
            }
        }
    }

    return *falhas;
}

