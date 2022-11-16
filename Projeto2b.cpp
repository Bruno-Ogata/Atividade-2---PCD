/*
Nome: Bruno Hideki Amadeu Ogata
RA: 140884
Programação Concorrente e Distribuída
Atividade 2 - Exercício 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string.h>

#define NUM_THREADS 8

typedef struct {
    int** geracao;
    int** prox;
    int tam;
} Grid;


int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970 
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}

void Inicializa_tabuleiro(Grid* grid, int tam) {
    int** geracao = (int**)malloc(tam * sizeof(int*));
    int** prox = (int**)malloc(tam * sizeof(int*));

    int i;
    for (i = 0; i < tam; i++) {
        geracao[i] = (int*)malloc(tam * sizeof(int));
        prox[i] = (int*)malloc(tam * sizeof(int));
    }

    grid->tam = tam;
    grid->geracao = geracao;
    grid->prox = prox;
}

void Preenche_tabuleiro(Grid* grid) {
    int i, j;
    for (i = 0; i < grid->tam; i++) {
        for (j = 0; j < grid->tam; j++) grid->geracao[i][j] = 0;
    }

    int lin = 1, col = 1;
    grid->geracao[lin][col + 1] = 1;
    grid->geracao[lin + 1][col + 2] = 1;
    grid->geracao[lin + 2][col] = 1;
    grid->geracao[lin + 2][col + 1] = 1;
    grid->geracao[lin + 2][col + 2] = 1;

    lin = 10; col = 30;
    grid->geracao[lin][col + 1] = 1;
    grid->geracao[lin][col + 2] = 1;
    grid->geracao[lin + 1][col] = 1;
    grid->geracao[lin + 1][col + 1] = 1;
    grid->geracao[lin + 2][col + 1] = 1;

}

int Soma_serial(Grid grid) {
    int count = 0, i, j;
    for (i = 0; i < grid.tam; i++) {
        for (j = 0; j < grid.tam; j++) {
            count = count + grid.geracao[i][j];
        }
    }
    return count;
}

int Soma_critical(Grid grid) {
    int cont = 0, k;

    int** geracao = grid.geracao;
    int** prox = grid.prox;
    int tam = grid.tam;

#pragma omp parallel private(k) shared (geracao, prox, tam, cont)
#pragma omp for
    for (k = 0; k < tam * tam; k++) {
        int i = floor(k / tam);
        int j = k % tam;

#pragma omp critical
        {
            cont += geracao[i][j];
        }
    }

    return cont;
}

int Soma_reduction(Grid grid) {
    int cont = 0, k;

    int** geracao = grid.geracao;
    int** prox = grid.prox;
    int tam = grid.tam;


#pragma omp parallel private(k) shared (geracao, prox, tam, cont)
#pragma omp for reduction(+: cont)
    for (k = 0; k < tam * tam; k++) {
        int i = floor(k / tam);
        int j = k % tam;

        cont += geracao[i][j];
    }


    return cont;
}

void Avanca_geracao(Grid* grid) {
    int** aux = grid->geracao;
    grid->geracao = grid->prox;
    grid->prox = aux;
}

int getNeighbors(int** geracao, int tam, int x, int y) {
    int cima = (y - 1 + tam) % tam;
    int baixo = (y + 1) % tam;
    int esq = (x - 1 + tam) % tam;
    int dir = (x + 1) % tam;

    return  geracao[esq][cima] + geracao[x][cima] + geracao[dir][cima] +
        geracao[esq][y] + 0 + geracao[dir][y] +
        geracao[esq][baixo] + geracao[x][baixo] + geracao[dir][baixo];
}

int main() {

    int N = 2048;
    int NUM_GENERATIONS = 2000;

    timeval start, end;
    float elapsed = 0;

    Grid grid;

    omp_set_num_threads(NUM_THREADS);

    // inicializa o tabuleiro
    Inicializa_tabuleiro(&grid, N);

    // preenche o tabuleiro
    Preenche_tabuleiro(&grid);

    printf("N de Threads        %d\n", NUM_THREADS);
    printf("Gerações do tabuleiro    %d\n", NUM_GENERATIONS);
    printf("N                  %d\n", N);
    printf("\n");


    gettimeofday(&start, NULL);

    for (int k = 0; k < NUM_GENERATIONS; k++) { 
        timeval localStart, localEnd;
        gettimeofday(&localStart, NULL);

        int i, j;

        int** geracao = grid.geracao;
        int** prox = grid.prox;
        int tam = grid.tam;

        // Divide a tarefa em Threads
#pragma omp parallel private(i, j) shared(geracao, prox, tam)
#pragma omp for collapse(2)
        for (i = 0; i <= tam - 1; i++) {
            for (j = 0; j <= tam - 1; j++) {
                int neighborhood = getNeighbors(geracao, tam, i, j);

                int controle = geracao[i][j];
                if (controle == 1) { // vivo
                    if (neighborhood < 2) controle = 0; // morre por abandono 
                    else if (neighborhood >= 4) controle = 0; // morre por superpopulação
                }
                else { // morto
                    if (neighborhood == 3) controle = 1; // revive
                }


                prox[i][j] = controle;
            }
        }

        gettimeofday(&localEnd, NULL);


        // próxima geração
        Avanca_geracao(&grid);

        timeval sumStart, sumEnd;
        gettimeofday(&sumStart, NULL);

        int localCount = Soma_reduction(grid);

        gettimeofday(&sumEnd, NULL);

        float localElapsed = (int)(1000 * (localEnd.tv_sec - localStart.tv_sec) + (localEnd.tv_usec - localStart.tv_usec) / 1000);
        float sumElapsed = (int)(1000 * (sumEnd.tv_sec - sumStart.tv_sec) + (sumEnd.tv_usec - sumStart.tv_usec) / 1000);

        printf("GEN %04d           %d        %.0f        %.0f\n", k + 1, localCount, localElapsed, sumElapsed);
    }

    gettimeofday(&end, NULL);
    elapsed = (int)(1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000);

    printf("elapsed time     %.2f\n", elapsed);

    return 0;
}