#include <stdio.h>    //  uso do printf, scanf, getchar, fgets, stdin e ferror
#include <stdlib.h>    // uso do rand, srand, system e exit.
#include <time.h>    // inicializar o sorteio aleatório.
#include <string.h>    // para verificar e tratar o nome digitado.

typedef struct
{
    int numero[15];
    int marcado[15]; // 1 para sorteado ; 0 para disponivel
} Cartela;

typedef struct
{
    int id;
    char nome[50];
    Cartela cartela;
    int venceu; // 0 se ainda esta jogando, 1 se ja "ganhou"
    int rodadaVitoria;
} Jogador;

void descartarRestanteLinha(void)
{
    int c;

    while ((c = getchar()) != '\n' && c != EOF)
    {
        // Descarta o restante da entrada
    }
}

int aguardaEnter(int *modoPular)
{
    // se verdadeiro - esta no loop, nao pede input
    if (*modoPular)
    {
        return 1;
    }

    printf("\nPressione ENTER para a proxima rodada");
    printf("\nPressione P para pular ate algum jogador ganhar\n");

    char c = getchar();
    if (c == '\n')
    {
        return 1; // continua, rodada a rodada
    }
    else if (c == 'P' || c == 'p')
    {
        *modoPular = 1; // ativa o modo salto automatico
        descartarRestanteLinha();
        return 1;
    }
    else
    {
        // valida os erros na entrada, descartando a linha, avisando o usuario e pedindo um novo input
        descartarRestanteLinha();
        printf("Entrada invalida, tente novamente!\n");
        return aguardaEnter(modoPular); // tenta de novo recursivamente
    }
}

void limparTela()
{
#if defined(_WIN32) || defined(_WIN64)
    system("cls"); // Executa no Windows antigo/padrão
#else
    system("clear"); // Executa no Linux e macOS
#endif
}

int recebeNumeroJogadores()
{
    int qtdJogadores;

    printf("Informe a quantidade de jogadores (2 a 5): "); // recebe a quantidade de jogadores

    if (scanf("%d", &qtdJogadores) != 1) // se o scanf leu algo diferente de "1 inteiro"
    {
        descartarRestanteLinha();
        printf("Entrada invalida! Digite um numero inteiro.\n");
        return recebeNumeroJogadores();
    }

    int c = getchar(); // variavel de auxilio para controle da validacao de entrada de caracteres

    if (c != '\n' && c != EOF) // valida se é um caracter aceito (numero inteiro)
    {
        descartarRestanteLinha(); // limpa buffer
        printf("Digite apenas um numero, sem letras.\n");
        return recebeNumeroJogadores(); // recursao
    }

    if (qtdJogadores < 2 || qtdJogadores > 5) // valida se esta no intervalo correto
    {
        printf("Digite um numero inteiro entre 2 e 5\n");
        return recebeNumeroJogadores(); // recursao
    }
    return qtdJogadores;
}

int sortearNumero(int numerosSorteados[], int qtsNumerosSorteados)
{
    if (qtsNumerosSorteados >= 60) // verifica se ja nao foram sorteados os 60 numeros
    {
        return 0;
    }

    int n = rand() % 60 + 1;
    int repetido = 1; // da a TAG inicialmente, para cair na validacao

    while (repetido)
    {
        repetido = 0; // retira momentaneamente a TAG, pois nem todo numero que entra no loop é repetido

        for (int i = 0; i < qtsNumerosSorteados; i++)
        {
            if (n == numerosSorteados[i]) // valida o numero
            {
                repetido = 1;        // se for repetido, volta novamente a dar a TAG
                n = rand() % 60 + 1; // gera um novo numero
                break;               // interrompe o FOR completo, reiniciando a validacao
            }
        }
    }
    return n;
}

void armazenaNumeroSorteado(int numerosSorteados[], int *qtsNumerosSorteados)
{
    if (*qtsNumerosSorteados >= 60) // valida se ja nao foram sorteados os 60 numeros (60 sorteios)
    {
        return;
    }

    int n = sortearNumero(numerosSorteados, *qtsNumerosSorteados);

    if (n == 0) // valida se a funcao nao returnou zero, caso de validacao da funcao para quando nao ha mais numeros disponiveis para sorteio
    {
        return;
    }

    numerosSorteados[*qtsNumerosSorteados] = n;
    (*qtsNumerosSorteados)++;
}

Cartela geraCartela(void)
{
    Cartela cartela; // inicializa a cartela

    for (int z = 0; z < 15; z++) // FOR para gerar os 15 numeros da cartela
    {
        int repetido = 1;          // da a TAG inicialmente, para cair na validacao
        int n = (rand() % 60 + 1); // gera um novo numero (ainda nao validado)

        while (repetido)
        {
            repetido = 0; // retira momentaneamente a TAG, pois nem todo numero que entra no loop é repetido

            for (int i = 0; i < z; i++) // 'z' seria == qtsNumerosGerados
            {
                if (n == cartela.numero[i]                                           // valida repeticao exata
                    || n == (cartela.numero[i] - 1) || n == (cartela.numero[i] + 1)) // valida repeticao consecutiva
                {
                    repetido = 1;        // se for repetido, volta novamente a ter a TAG, para reiniciar a validacao
                    n = rand() % 60 + 1; // gera um novo numero (ainda nao validado)
                    break;               // interrompe o FOR completo
                }
            }
        }
        cartela.numero[z] = n;  // atribui o numero a cartela
        cartela.marcado[z] = 0; // marca os numeros como "nao marcados"
    }
    return cartela;
}

int leNome(char nome[], int tamanho)
{
    while (1){

    printf("Digite o nome do jogador: ");

    if (fgets(nome, tamanho, stdin) == NULL)    // se nao conseguiu ler
    {
        printf("\nErro ao ler o nome.\n");
        exit(1);   // indica falha e exita do codigo
    }

    int tamanhoNome = strlen(nome);

    if (tamanhoNome > 0 && nome[tamanhoNome - 1] == '\n') // se tiver nome, e ele estiver com o '\n'
    {
        nome[tamanhoNome - 1] = '\0'; // coloca "final" na string nome, com o '\0' no lugar do '\n'
    }
    else
    {
        int c = getchar();

        if (c != '\n' && c != EOF) // verifica se o nome passou do limite das 49 caracteres
        {
            descartarRestanteLinha();
            printf("Nome muito longo. Tente novamente.\n");

            continue;   // volta para o inicio do laço, para pedir outro nome
        }

        if (c == EOF && ferror(stdin))  // verifica se ocorreu um erro na leitura ou encontrou o fim da entrada
        {
            exit(1);   // indica falha e exita do codigo
        }
    }

    if (nome[0] == '\0')    // verficia se a primeira letra do nome esta vazia (indicativo de que o nome nao foi preenchido)
    {
        printf("O nome nao pode ficar vazio.\n");

        continue;   // volta para o inicio do laço, para pedir outro nome
    }

    return 1;
    }
}

int cadastraJogador(Jogador players[], int qtdJogadores)
{
    for (int i = 0; i < qtdJogadores; i++)
    {
        if (!leNome(players[i].nome, sizeof(players[i].nome)))
        {
            return 0;
        }

        players[i].id = i + 1;
        players[i].cartela = geraCartela();
        players[i].venceu = 0;
        players[i].rodadaVitoria = 0;
    }

    return 1;
}

void exibeCartela(Jogador jogador)
{
    printf("\nCartela de Bingo - %s (ID: %d)\n", jogador.nome, jogador.id);

    for (int i = 0; i < 15; i++)
    {
        if (i % 5 == 0)
            printf("|----|----|----|----|----|\n");

        if (jogador.cartela.marcado[i])
        {
            printf("|%2d* ", jogador.cartela.numero[i]); // o asterisco '*' indica que o numero foi marcado (preciso melhorar - nao sei como)
        }
        else
        {
            printf("|%2d  ", jogador.cartela.numero[i]); // sem o asterisco (nao é um numero ja sorteado)
        }

        if ((i + 1) % 5 == 0)
            printf("|\n");
    }
    printf("|----|----|----|----|----|\n");
}

void marcaNumero(Jogador players[], int qtdJogadores, int numeroSorteado)
{
    for (int i = 0; i < qtdJogadores; i++)
    {
        if (players[i].venceu == 1) // skipa os players que ja venceram
        {
            continue; // comando para "pular" (!= do break, que para)
        }

        for (int j = 0; j < 15; j++)
        {
            if (players[i].cartela.numero[j] == numeroSorteado)
            {
                players[i].cartela.marcado[j] = 1; // marca o da tabela da pessoa se o numeroTabela == numeroSorteado
            }
        }
    }
}

void exibeResumoRodada(int rodada, int numeroSorteado)
{
    printf("\n========== BINGO ==========\n");
    printf("Rodada: %d/60\n", rodada);
    printf("Numero sorteado: %d\n", numeroSorteado);
}

int verificaVitoria(Jogador *jogador, int rodada) // usa ponteiro para refenciar diretamente ao jogador
{
    for (int i = 0; i < 15; i++)
    {
        if (jogador->cartela.marcado[i] == 0) // usa '->' para acessar o ponteiro referente
        {
            return 0;
        }
    }
    if (jogador->venceu == 0)
    {
        // se ele nao venceu ainda
        jogador->venceu = 1;
        jogador->rodadaVitoria = rodada;
    }
    return 1;
}

void exibeResultado(Jogador players[], int qtdJogadores, int jogadoresVencedores, int jogoEncerrado, int rodadaAtual)
{
    // se o jogo ja tiver acabado
    if (jogoEncerrado)
    {
        // exibe o ranking ordenado do primeiro ao último (varrendo as rodadas de 1 a 60)
        printf("\n========== RESULTADO FINAL - RANKING ==========\n");
        int posicao = 1;
        for (int rodada = 1; rodada <= 60; rodada++)
        {
            for (int i = 0; i < qtdJogadores; i++)
            {
                if (players[i].venceu && players[i].rodadaVitoria == rodada)
                {
                    printf("%d Lugar: %s (ID: %i) - vencendo na rodada %d\n", posicao, players[i].nome, players[i].id, players[i].rodadaVitoria);
                    posicao++;
                }
            }
        }
    }
    // se ainda tiver jogo
    else
    {
        printf("\n========== STATUS PARCIAL DO JOGO ==========\n");
        printf("Quantos jogadores ja venceram: %d de %d\n", jogadoresVencedores, qtdJogadores);

        // verifica e exibe quem venceu em rodadas anteriores
        int venceramAnteriores = 0;
        for (int i = 0; i < qtdJogadores; i++)
        {
            if (players[i].venceu && players[i].rodadaVitoria < rodadaAtual)
            {
                venceramAnteriores = 1; // atribui como 'V'
                break;
            }
        }
        // se condicao verdadeira
        if (venceramAnteriores)
        {
            printf("\nJa venceram em rodadas anteriores:\n");
            for (int i = 0; i < qtdJogadores; i++)
            {
                if (players[i].venceu && players[i].rodadaVitoria < rodadaAtual)
                {
                    printf("   - %s (ID: %i) - Venceu na rodada %d\n", players[i].nome, players[i].id, players[i].rodadaVitoria);
                }
            }
        }
    }
}

int main(void)
{
    int qtsNumerosSorteados = 0;
    int numerosSorteados[60];
    int jogadoresVencedores = 0;
    int qtdJogadores = recebeNumeroJogadores();
    int modoPular = 0; // variavel de controle para a opcao p

    srand(time(NULL));

    Jogador players[qtdJogadores];
    cadastraJogador(players, qtdJogadores);

    for (int z = 1; z <= 60 && jogadoresVencedores < qtdJogadores; z++)
    {
        armazenaNumeroSorteado(numerosSorteados, &qtsNumerosSorteados);
        int numeroSorteado = numerosSorteados[z - 1];

        marcaNumero(players, qtdJogadores, numeroSorteado);
        limparTela(); // limpa a tela para a nova rodada

        exibeResumoRodada(z, numeroSorteado);

        int novoVencedor = 0; // variavel de controle para se alguem venceu agora

        for (int i = 0; i < qtdJogadores; i++) // verifica se alguem ganhou nesta rodada
        {
            if (players[i].venceu == 0 && verificaVitoria(&players[i], z))
            {
                printf("\nBINGO! %s venceu!!!\n", players[i].nome);
                jogadoresVencedores++;
                novoVencedor = 1; // indica que alguem ganhou
            }
        }
        // se alguem venceu nesta rodada, desativa o modo pular para pausar o jogo
        if (novoVencedor)
        {
            modoPular = 0;
        }

        if (jogadoresVencedores < qtdJogadores) // se o jogo ainda nao acabou para todos
        {
            printf("\n--- Cartelas em jogo ---\n");

            for (int i = 0; i < qtdJogadores; i++)
            {
                if (players[i].venceu == 0)
                {
                    exibeCartela(players[i]);
                }
            }

            exibeResultado(players, qtdJogadores, jogadoresVencedores, 0, z);

            if (z < 60)
            {
                aguardaEnter(&modoPular); // passa o endereco do valor original
            }
        }
    }

    // exibe o ranking final definitivo ao termino do jogo
    exibeResultado(players, qtdJogadores, jogadoresVencedores, 1, 60);

    return 0;
}
