#include "../bingo_engine.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"Falha %s:%d: %s\n",__FILE__,__LINE__,#x); return 0; } } while(0)
static int lifecycle(void) {
 BingoGame g; const char *names[MAX_JOGADORES]={"Ana","Bia","Caio","Duda","Eva"};
 int n=0, original[TAMANHO_CARTELA],i;
 bingoat_seed(12345U);
 CHECK(!bingo_game_init(&g,1,names,1)); CHECK(!bingo_game_init(&g,6,names,1));
 CHECK(bingo_game_init(&g,3,names,1)); CHECK(g.qtdJogadores==3);
 memcpy(original,g.players[0].cartela.numero,sizeof original);
 CHECK(bingo_game_draw(&g,&n)); CHECK(n>=1&&n<=60); CHECK(g.qtdNumerosSorteados==1);
 CHECK(bingo_game_undo(&g)); CHECK(g.qtdNumerosSorteados==0); CHECK(!bingo_game_undo(&g));
 bingo_game_reset(&g,0); CHECK(!memcmp(original,g.players[0].cartela.numero,sizeof original));
 CHECK(bingo_game_toggle_board(&g,60)); CHECK(g.numerosSorteados[0]==60);
 CHECK(bingo_game_toggle_board(&g,60)); CHECK(g.qtdNumerosSorteados==0);
 CHECK(!bingo_game_toggle_board(&g,0)); CHECK(!bingo_game_toggle_card(&g,0,1));
 CHECK(bingo_game_toggle_card(&g,1,original[0])); CHECK(!g.autoMark);
 CHECK(bingo_game_sync(&g,5,names,0,1)); CHECK(g.qtdJogadores==5);
 CHECK(bingo_game_sync(&g,2,names,1,0)); CHECK(g.qtdJogadores==2);
 bingo_game_reset(&g,1);
 for(i=0;i<g.qtdJogadores;i++) CHECK(!g.players[i].venceu&&!g.players[i].rodadaVitoria);
 return 1;
}
int main(void){ if(!lifecycle()) return 1; puts("API publica do motor: OK"); return 0; }
