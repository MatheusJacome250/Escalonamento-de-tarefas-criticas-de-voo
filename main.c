#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct tarefa {
    char nome[50];
    int periodo;
    int deadline;
    int burst;

    int tempo_restante;
    int deadline_absoluto;
    int proxima_chegada;

    int completas;
    int perdidas;
    int killed;

    struct tarefa *prox;
} Tarefa;


void inserirFim(Tarefa **inicio, Tarefa **fim, Tarefa *nova) {

    nova->prox = NULL;

    if (*inicio == NULL) {
        *inicio = nova;
        *fim = nova;
    } else {
        (*fim)->prox = nova;
        *fim = nova;
    }
}

void liberarLista(Tarefa *inicio) {
    Tarefa *atual = inicio;

    while (atual != NULL) {
        Tarefa *proxima = atual->prox;
        free(atual);
        atual = proxima;
    }
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Erro: numero incorreto de argumentos.\n");
        return 1;
    }

    if (strcmp(argv[1], "rate") != 0 && strcmp(argv[1], "edf") != 0) {
        fprintf(stderr, "Erro: algoritmo invalido.\n");
        return 1;
    }

    FILE *arquivo = fopen(argv[2], "r");

    if (arquivo == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir o arquivo de entrada.\n");
        return 1;
    }

    int tempo_total;

    if (fscanf(arquivo, "%d", &tempo_total) != 1) {
        fprintf(stderr, "Erro: arquivo malformado.\n");
        fclose(arquivo);
        return 1;
    }

    if (tempo_total <= 0) {
        fprintf(stderr, "Erro: tempo total invalido.\n");
        fclose(arquivo);
        return 1;
    }

    Tarefa *inicio = NULL;
    Tarefa *fim = NULL;


    while (1) {
        Tarefa *nova = (Tarefa *)malloc(sizeof(Tarefa));

        if (nova == NULL) {
            fprintf(stderr, "Erro: falha de memoria.\n");
            fclose(arquivo);
            return 1;
        }

        int lidos = fscanf(arquivo, "%49s %d %d %d",nova->nome, &nova->periodo, &nova->deadline, &nova->burst);

        if (lidos == EOF) {
            free(nova);
            break;
        }

        if (lidos != 4) {
            fprintf(stderr, "Erro: arquivo malformado.\n");
            free(nova);
            fclose(arquivo);
            return 1;
        }

        if (nova->periodo <= 0 || nova->deadline <= 0 || nova->burst <= 0) {
            fprintf(stderr, "Erro: valor invalido.\n");
            free(nova);
            fclose(arquivo);
            return 1;
        }

        if (nova->deadline > nova->periodo || nova->burst > nova->deadline) {
            fprintf(stderr, "Erro: tarefa invalida.\n");
            free(nova);
            fclose(arquivo);
            return 1;
        }

        nova->tempo_restante = 0;
        nova->deadline_absoluto = 0;
        nova->proxima_chegada = 0;

        nova->completas = 0;
        nova->perdidas = 0;
        nova->killed = 0;

        inserirFim(&inicio, &fim, nova);
    }

    fclose(arquivo);

    for (int tempo = 0; tempo < tempo_total; tempo++) {

        Tarefa *atual = inicio;

        while (atual != NULL) {

            if (atual->tempo_restante > 0 && tempo == atual->deadline_absoluto) {

                atual->perdidas++;
                atual->tempo_restante = 0;
            }

            if (tempo == atual->proxima_chegada) {

                atual->tempo_restante = atual->burst;
                atual->deadline_absoluto = tempo + atual->deadline;
                atual->proxima_chegada = tempo + atual->periodo;
            }

            atual = atual->prox;
        }
    }

    Tarefa *atual = inicio;

    while (atual != NULL) {

        if (atual->tempo_restante > 0) {

            if (atual->deadline_absoluto == tempo_total) {
                atual->perdidas++;
            } else {
                atual->killed++;
            }

            atual->tempo_restante = 0;
        }

        atual = atual->prox;
    }

    liberarLista(inicio);
    return 0;
}