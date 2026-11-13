#ifndef BINGO_ENGINE_H
#define BINGO_ENGINE_H
#include <stddef.h>
#define MAX_NUMERO_BINGO 60
#define TAMANHO_CARTELA 15
#define MIN_JOGADORES 2
#define MAX_JOGADORES 5
#define CARTELA_LINHAS 3
#define CARTELA_COLUNAS 5
#define MAX_LINHA_NUMEROS 15
#define MAX_LINHAS_ATIVAS 48
typedef struct { int numero[TAMANHO_CARTELA]; int marcado[TAMANHO_CARTELA]; } Cartela;
typedef struct { int id; char nome[50]; Cartela cartela; int venceu; int rodadaVitoria; } Jogador;
typedef struct { int orientacao; int indiceA; int indiceB; int tamanho; int numeros[MAX_LINHA_NUMEROS]; } LinhaCompleta;
typedef struct {
 Jogador players[MAX_JOGADORES]; int qtdJogadores;
 int numerosSorteados[MAX_NUMERO_BINGO]; int qtdNumerosSorteados; int autoMark;
} BingoGame;
void bingoat_seed(unsigned int seed);
int sortearNumero(int *, int);
void armazenaNumeroSorteado(int *, int *);
void ordenarCartela(Cartela *);
void geraCartela(Cartela *);
void marcaNumero(Jogador *, int, int);
int verificaVitoria(Jogador *, int);
int estaNoConjunto(int, int *, int);
int detectarLinhasGrade(int *, int *, int, int, int, LinhaCompleta *, int);
void sincronizarMarcacoesAutomaticas(Jogador *, int, int *, int);
void atualizarVencedores(Jogador *, int, int);
int bingo_game_init(BingoGame *, int, const char *const [], int);
int bingo_game_draw(BingoGame *, int *);
int bingo_game_toggle_board(BingoGame *, int);
int bingo_game_toggle_card(BingoGame *, int, int);
int bingo_game_sync(BingoGame *, int, const char *const [], int, int);
int bingo_game_undo(BingoGame *);
void bingo_game_reset(BingoGame *, int);
int bingo_game_finished(const BingoGame *);
#endif
