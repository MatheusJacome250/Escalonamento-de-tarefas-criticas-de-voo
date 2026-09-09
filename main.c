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

Tarefa *escolherRate(Tarefa *inicio) {
    Tarefa *escolhida = NULL;
    Tarefa *atual = inicio;

    while (atual != NULL) {

        if (atual->tempo_restante > 0) {

            if (escolhida == NULL || atual->periodo < escolhida->periodo) {
                escolhida = atual;
            }
        }

        atual = atual->prox;
    }

    return escolhida;
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

    FILE *saida = fopen("rate_masj.out", "w");

    if (saida == NULL) {
        fprintf(stderr, "Erro: nao foi possivel criar o arquivo de saida.\n");
        liberarLista(inicio);
        return 1;
    }

    fprintf(saida, "EXECUTION BY RATE\n");

    Tarefa *anterior = NULL;
    int unidades_bloco = 0;
    int unidades_idle = 0;

    for (int tempo = 0; tempo < tempo_total; tempo++) {

        Tarefa *atual = inicio;

        while (atual != NULL) {

            if (atual->tempo_restante > 0 && tempo == atual->deadline_absoluto) {

                if (atual == anterior && unidades_bloco > 0) {
                    fprintf(saida, "[%s] for %d units - L\n",
                            atual->nome, unidades_bloco);

                    anterior = NULL;
                    unidades_bloco = 0;
                }

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

        if (strcmp(argv[1], "rate") == 0) {
            Tarefa *executando = escolherRate(inicio);

            if (executando != NULL) {

                if (unidades_idle > 0) {
                    fprintf(saida, "idle for %d units\n", unidades_idle);
                    unidades_idle = 0;
                }

                if (anterior == NULL) {

                    anterior = executando;
                    unidades_bloco = 0;

                } else if (anterior != executando) {

                    fprintf(saida, "[%s] for %d units - H\n",
                            anterior->nome, unidades_bloco);

                    anterior = executando;
                    unidades_bloco = 0;
                }

                unidades_bloco++;

                executando->tempo_restante--;

                if (executando->tempo_restante == 0) {

                    executando->completas++;

                    fprintf(saida, "[%s] for %d units - F\n",
                            executando->nome, unidades_bloco);

                    anterior = NULL;
                    unidades_bloco = 0;
                }

            } else {

                unidades_idle++;
            }

        }
    }
    if (unidades_idle > 0) {
        fprintf(saida, "idle for %d units\n", unidades_idle);
    }
    Tarefa *atual = inicio;

    while (atual != NULL) {
        if (atual->tempo_restante > 0) {

            if (atual->deadline_absoluto == tempo_total) {

                if (atual == anterior && unidades_bloco > 0) {
                    fprintf(saida, "[%s] for %d units - L\n",
                            atual->nome, unidades_bloco);

                    anterior = NULL;
                    unidades_bloco = 0;
                }

                atual->perdidas++;

            } else {

                if (atual == anterior && unidades_bloco > 0) {
                    fprintf(saida, "[%s] for %d units - K\n",
                            atual->nome, unidades_bloco);

                    anterior = NULL;
                    unidades_bloco = 0;
                }

                atual->killed++;
            }

            atual->tempo_restante = 0;
        }

        atual = atual->prox;
    }
    fprintf(saida, "LOST DEADLINES\n");
    atual = inicio;
    while (atual != NULL) {
        fprintf(saida, "[%s] %d\n", atual->nome, atual->perdidas);
        atual = atual->prox;
    }

    fprintf(saida, "COMPLETE EXECUTION\n");

    atual = inicio;

    while (atual != NULL) {
        fprintf(saida, "[%s] %d\n", atual->nome, atual->completas);
        atual = atual->prox;
    }

    fprintf(saida, "KILLED\n");

    atual = inicio;

    while (atual != NULL) {
        fprintf(saida, "[%s] %d\n", atual->nome, atual->killed);
        atual = atual->prox;
    }
    fclose(saida);
    liberarLista(inicio);
    return 0;
}