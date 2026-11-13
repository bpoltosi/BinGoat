#include "bingo_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void bingoat_seed(unsigned int seed) { srand(seed); }
int sortearNumero(int *numerosSorteados, int qtsNumerosSorteados)
{
    int n;
    int repetido;
    int i;

    if (qtsNumerosSorteados >= MAX_NUMERO_BINGO)
    {
        return 0;
    }

    n = rand() % MAX_NUMERO_BINGO + 1;
    repetido = 1;

    while (repetido)
    {
        repetido = 0;

        for (i = 0; i < qtsNumerosSorteados; i++)
        {
            if (n == numerosSorteados[i])
            {
                repetido = 1;
                n = rand() % MAX_NUMERO_BINGO + 1;
                break;
            }
        }
    }
    return n;
}

void armazenaNumeroSorteado(int *numerosSorteados, int *qtsNumerosSorteados)
{
    int n;

    if (*qtsNumerosSorteados >= MAX_NUMERO_BINGO)
    {
        return;
    }

    n = sortearNumero(numerosSorteados, *qtsNumerosSorteados);
    if (n == 0)
    {
        return;
    }

    numerosSorteados[*qtsNumerosSorteados] = n;
    (*qtsNumerosSorteados)++;
}

void ordenarCartela(Cartela *cartela)
{
    int i;
    int j;
    int tmpNum;
    int tmpMarc;

    for (i = 0; i < TAMANHO_CARTELA - 1; i++)
    {
        for (j = i + 1; j < TAMANHO_CARTELA; j++)
        {
            if (cartela->numero[j] < cartela->numero[i])
            {
                tmpNum = cartela->numero[i];
                cartela->numero[i] = cartela->numero[j];
                cartela->numero[j] = tmpNum;

                tmpMarc = cartela->marcado[i];
                cartela->marcado[i] = cartela->marcado[j];
                cartela->marcado[j] = tmpMarc;
            }
        }
    }
}

void geraCartela(Cartela *cartela)
{
    int z;
    int i;
    int repetido;
    int n;

    for (z = 0; z < TAMANHO_CARTELA; z++)
    {
        repetido = 1;
        n = rand() % MAX_NUMERO_BINGO + 1;

        while (repetido)
        {
            repetido = 0;

            for (i = 0; i < z; i++)
            {
                if (n == cartela->numero[i] ||
                    n == (cartela->numero[i] - 1) ||
                    n == (cartela->numero[i] + 1))
                {
                    repetido = 1;
                    n = rand() % MAX_NUMERO_BINGO + 1;
                    break;
                }
            }
        }
        cartela->numero[z] = n;
        cartela->marcado[z] = 0;
    }

    ordenarCartela(cartela);
}

void marcaNumero(Jogador *players, int qtdJogadores, int numeroSorteado)
{
    int i;
    int j;

    for (i = 0; i < qtdJogadores; i++)
    {
        if (players[i].venceu == 1)
        {
            continue;
        }

        for (j = 0; j < TAMANHO_CARTELA; j++)
        {
            if (players[i].cartela.numero[j] == numeroSorteado)
            {
                players[i].cartela.marcado[j] = 1;
            }
        }
    }
}

int verificaVitoria(Jogador *jogador, int rodada)
{
    int i;

    for (i = 0; i < TAMANHO_CARTELA; i++)
    {
        if (jogador->cartela.marcado[i] == 0)
        {
            return 0;
        }
    }
    if (jogador->venceu == 0)
    {
        jogador->venceu = 1;
        jogador->rodadaVitoria = rodada > 0 ? rodada : 1;
    }
    return 1;
}

int estaNoConjunto(int valor, int *lista, int tamanho)
{
    int i;
    for (i = 0; i < tamanho; i++)
    {
        if (lista[i] == valor)
        {
            return 1;
        }
    }
    return 0;
}

int detectarLinhasGrade(
    int *valores,
    int *marcados,
    int totalValores,
    int linhas,
    int colunas,
    LinhaCompleta *saida,
    int maxSaida
)
{
    int count = 0;
    int r;
    int c;
    int k;
    int idx;
    int completa;
    int diagLen;

    /* Horizontais */
    for (r = 0; r < linhas && count < maxSaida; r++)
    {
        completa = 1;
        for (c = 0; c < colunas; c++)
        {
            idx = r * colunas + c;
            if (idx >= totalValores || !marcados[idx])
            {
                completa = 0;
                break;
            }
            saida[count].numeros[c] = valores[idx];
        }
        if (completa)
        {
            saida[count].orientacao = 0;
            saida[count].indiceA = r;
            saida[count].indiceB = 0;
            saida[count].tamanho = colunas;
            count++;
        }
    }

    /* Verticais */
    for (c = 0; c < colunas && count < maxSaida; c++)
    {
        completa = 1;
        for (r = 0; r < linhas; r++)
        {
            idx = r * colunas + c;
            if (idx >= totalValores || !marcados[idx])
            {
                completa = 0;
                break;
            }
            saida[count].numeros[r] = valores[idx];
        }
        if (completa)
        {
            saida[count].orientacao = 1;
            saida[count].indiceA = c;
            saida[count].indiceB = 0;
            saida[count].tamanho = linhas;
            count++;
        }
    }

    /* Diagonais de comprimento min(linhas, colunas) */
    diagLen = linhas < colunas ? linhas : colunas;
    if (diagLen >= 2)
    {
        /* Diagonal principal (\) */
        for (r = 0; r <= linhas - diagLen && count < maxSaida; r++)
        {
            for (c = 0; c <= colunas - diagLen && count < maxSaida; c++)
            {
                completa = 1;
                for (k = 0; k < diagLen; k++)
                {
                    idx = (r + k) * colunas + (c + k);
                    if (idx >= totalValores || !marcados[idx])
                    {
                        completa = 0;
                        break;
                    }
                    saida[count].numeros[k] = valores[idx];
                }
                if (completa)
                {
                    saida[count].orientacao = 2;
                    saida[count].indiceA = r;
                    saida[count].indiceB = c;
                    saida[count].tamanho = diagLen;
                    count++;
                }
            }
        }

        /* Diagonal secundaria (/) */
        for (r = diagLen - 1; r < linhas && count < maxSaida; r++)
        {
            for (c = 0; c <= colunas - diagLen && count < maxSaida; c++)
            {
                completa = 1;
                for (k = 0; k < diagLen; k++)
                {
                    idx = (r - k) * colunas + (c + k);
                    if (idx >= totalValores || !marcados[idx])
                    {
                        completa = 0;
                        break;
                    }
                    saida[count].numeros[k] = valores[idx];
                }
                if (completa)
                {
                    saida[count].orientacao = 3;
                    saida[count].indiceA = r;
                    saida[count].indiceB = c;
                    saida[count].tamanho = diagLen;
                    count++;
                }
            }
        }
    }

    return count;
}
void sincronizarMarcacoesAutomaticas(
    Jogador *players,
    int qtdJogadores,
    int *numerosSorteados,
    int qtsNumerosSorteados
)
{
    int i;
    int j;

    for (i = 0; i < qtdJogadores; i++)
    {
        for (j = 0; j < TAMANHO_CARTELA; j++)
        {
            if (estaNoConjunto(players[i].cartela.numero[j], numerosSorteados, qtsNumerosSorteados))
            {
                players[i].cartela.marcado[j] = 1;
            }
            else
            {
                players[i].cartela.marcado[j] = 0;
            }
        }
    }
}

void atualizarVencedores(Jogador *players, int qtdJogadores, int rodadaAtual)
{
    int i;
    int j;
    int todosMarcados;

    for (i = 0; i < qtdJogadores; i++)
    {
        todosMarcados = 1;
        for (j = 0; j < TAMANHO_CARTELA; j++)
        {
            if (players[i].cartela.marcado[j] == 0)
            {
                todosMarcados = 0;
                break;
            }
        }

        if (todosMarcados)
        {
            if (players[i].venceu == 0)
            {
                players[i].venceu = 1;
                players[i].rodadaVitoria = rodadaAtual > 0 ? rodadaAtual : 1;
            }
        }
        else
        {
            players[i].venceu = 0;
            players[i].rodadaVitoria = 0;
        }
    }
}
static int valid_game(const BingoGame *g) {
 return g && g->qtdJogadores >= MIN_JOGADORES && g->qtdJogadores <= MAX_JOGADORES &&
 g->qtdNumerosSorteados >= 0 && g->qtdNumerosSorteados <= MAX_NUMERO_BINGO;
}
static void update_game(BingoGame *g) {
 int i;
 if (g->autoMark) sincronizarMarcacoesAutomaticas(g->players,g->qtdJogadores,g->numerosSorteados,g->qtdNumerosSorteados);
 atualizarVencedores(g->players,g->qtdJogadores,g->qtdNumerosSorteados);
 for(i=0;i<g->qtdJogadores;i++) g->players[i].id=i+1;
}
int bingo_game_init(BingoGame *g,int count,const char *const names[],int autoMark) {
 int i;
 if(!g || count<MIN_JOGADORES || count>MAX_JOGADORES) return 0;
 memset(g,0,sizeof(*g)); g->qtdJogadores=count; g->autoMark=autoMark!=0;
 for(i=0;i<count;i++) {
  g->players[i].id=i+1;
  if(names && names[i] && names[i][0]) strncpy(g->players[i].nome,names[i],sizeof(g->players[i].nome)-1);
  else snprintf(g->players[i].nome,sizeof(g->players[i].nome),"Jogador %d",i+1);
  geraCartela(&g->players[i].cartela);
 }
 return 1;
}
int bingo_game_draw(BingoGame *g,int *drawn) {
 int before;
 if(!valid_game(g)||g->qtdNumerosSorteados>=MAX_NUMERO_BINGO) return 0;
 before=g->qtdNumerosSorteados; armazenaNumeroSorteado(g->numerosSorteados,&g->qtdNumerosSorteados);
 if(g->qtdNumerosSorteados!=before+1) return 0;
 if(drawn) *drawn=g->numerosSorteados[before]; update_game(g); return 1;
}
int bingo_game_toggle_board(BingoGame *g,int n) {
 int i;
 if(!valid_game(g)||n<1||n>MAX_NUMERO_BINGO) return 0;
 for(i=0;i<g->qtdNumerosSorteados;i++) if(g->numerosSorteados[i]==n) {
  memmove(&g->numerosSorteados[i],&g->numerosSorteados[i+1],(size_t)(g->qtdNumerosSorteados-i-1)*sizeof(int));
  g->qtdNumerosSorteados--; update_game(g); return 1;
 }
 if(g->qtdNumerosSorteados>=MAX_NUMERO_BINGO) return 0;
 g->numerosSorteados[g->qtdNumerosSorteados++]=n; update_game(g); return 1;
}
int bingo_game_toggle_card(BingoGame *g,int playerId,int n) {
 int i;
 if(!valid_game(g)||playerId<1||playerId>g->qtdJogadores||n<1||n>MAX_NUMERO_BINGO) return 0;
 for(i=0;i<TAMANHO_CARTELA;i++) if(g->players[playerId-1].cartela.numero[i]==n) {
  g->players[playerId-1].cartela.marcado[i]=!g->players[playerId-1].cartela.marcado[i];
  g->autoMark=0; atualizarVencedores(g->players,g->qtdJogadores,g->qtdNumerosSorteados); return 1;
 }
 return 0;
}
int bingo_game_sync(BingoGame *g,int count,const char *const names[],int regenerate,int autoMark) {
 int i,old;
 if(!valid_game(g)||count<MIN_JOGADORES||count>MAX_JOGADORES) return 0;
 old=g->qtdJogadores;
 for(i=count;i<old;i++) memset(&g->players[i],0,sizeof(Jogador));
 for(i=0;i<count;i++) {
  if(i>=old||regenerate) { memset(&g->players[i],0,sizeof(Jogador)); g->players[i].id=i+1; geraCartela(&g->players[i].cartela); }
  g->players[i].id=i+1;
  if(names&&names[i]&&names[i][0]) { strncpy(g->players[i].nome,names[i],sizeof(g->players[i].nome)-1); g->players[i].nome[sizeof(g->players[i].nome)-1]='\0'; }
 }
 g->qtdJogadores=count; g->autoMark=autoMark!=0; update_game(g); return 1;
}
int bingo_game_undo(BingoGame *g) {
 if(!valid_game(g)||g->qtdNumerosSorteados==0) return 0;
 g->qtdNumerosSorteados--; update_game(g); return 1;
}
void bingo_game_reset(BingoGame *g,int newCards) {
 int i;
 if(!valid_game(g)) return;
 g->qtdNumerosSorteados=0; memset(g->numerosSorteados,0,sizeof(g->numerosSorteados));
 for(i=0;i<g->qtdJogadores;i++) {
  memset(g->players[i].cartela.marcado,0,sizeof(g->players[i].cartela.marcado));
  g->players[i].venceu=0; g->players[i].rodadaVitoria=0;
  if(newCards) geraCartela(&g->players[i].cartela);
 }
}
int bingo_game_finished(const BingoGame *g) {
 int i; if(!valid_game(g)) return 0;
 if(g->qtdNumerosSorteados>=MAX_NUMERO_BINGO) return 1;
 for(i=0;i<g->qtdJogadores;i++) if(!g->players[i].venceu) return 0;
 return 1;
}
