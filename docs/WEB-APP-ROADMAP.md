# Plano de desenvolvimento — versão web do BinGoat

> Documento de planejamento. Esta proposta não implementa a versão web nem altera o motor C atual.

## 1. Objetivo

Evoluir o BinGoat, hoje executado no terminal, para uma aplicação acessível pelo navegador, preservando o **C como única fonte de verdade das regras do jogo**.

A interface terá visual 2D simples:
- uma máquina de bingo ilustrada em SVG, com animações curtas da alavanca, do globo, da portinha e da bola sorteada;
- uma cartela visual para cada jogador, com números gerados pelo backend C;
- atualização das marcações, histórico de sorteios e indicação de vencedores.

O navegador será responsável por apresentar e animar os dados. Ele não gerará cartelas, não sorteará números e não decidirá vitórias.

## 2. Escopo funcional da primeira versão

### 2.1 Regras preservadas

A primeira versão web deve manter as regras já existentes no programa:
- de 2 a 5 jogadores por partida;
- 15 números por cartela, organizados em 3 linhas e 5 colunas;
- números de 1 a 60;
- sem números repetidos ou consecutivos dentro da mesma cartela;
- sorteios sem repetição;
- marcação automática dos números sorteados;
- vitória quando os 15 números da cartela estiverem marcados;
- encerramento quando todos os jogadores vencerem ou os 60 números forem sorteados.

**Importante:** a aparência pode ser inspirada em uma cartela tradicional de bingo, mas o formato funcional inicial continua sendo o atual, de 3 × 5 (15 posições). Não faz parte deste escopo migrar para cartelas 5 × 5 ou para uma modalidade de 75 bolas.

### 2.2 Cartelas dinâmicas

O desenho da cartela será estático; os dados serão dinâmicos.

1. O usuário informa os nomes dos jogadores e cria uma partida.
2. O backend C gera uma cartela individual para cada jogador.
3. A API devolve os números gerados e o estado inicial de marcação.
4. O frontend monta uma cartela visual para cada jogador com esses dados.
5. Durante a partida, os números permanecem fixos; somente o estado de marcação muda.
6. Uma nova partida recebe novas cartelas geradas pelo backend.

A interface deve manter as cartelas dos jogadores visíveis durante a partida e destacar as que já venceram, sem alterar os números que receberam.

### 2.3 Máquina e animação 2D

A máquina será uma ilustração SVG composta por elementos independentes:
- estrutura/base;
- globo;
- bolas internas;
- alavanca;
- portinha de saída;
- bocal/canal de saída;
- bola sorteada, com número dinâmico.

Sequência visual pretendida:
1. O usuário aciona **Próxima rodada / Sortear**.
2. A alavanca desce e retorna.
3. O globo gira por um curto período.
4. O globo para e a portinha abre.
5. A bola sai pela abertura e vai para uma área de destaque.
6. O número real retornado pelo backend aparece na bola.
7. As cartelas e o histórico são atualizados e, se houver, o vencedor é destacado.

A sequência deve durar aproximadamente 2 a 3 segundos, com possibilidade de ajuste visual. O estilo é 2D, sem modelagem 3D, física, WebGL ou simulação realista.

## 3. Arquitetura proposta

```text
Navegador
  HTML + CSS + JavaScript
  SVG da máquina e cartelas dinâmicas
          |
          | HTTP / JSON
          v
Servidor HTTP em C (CivetWeb)
          |
          +-- cJSON: leitura e escrita JSON
          |
          +-- Motor do bingo em C
                - estado da partida
                - geração de cartelas
                - sorteio
                - marcação
                - verificação de vitória
                - ranking
```

### 3.1 Backend

- **C:** mantém o motor e todas as regras do jogo.
- **CivetWeb:** servidor HTTP embutido em C.
- **cJSON:** serialização e leitura das mensagens JSON.
- **Estado em memória:** uma partida ativa por processo nesta primeira versão.
- **Sem banco de dados:** não haverá persistência após reiniciar o servidor.

O código atual está concentrado em `TrabalhoBingo.c` e mistura regras com entrada/saída de terminal. A primeira etapa será separar o motor da interface de terminal, preservando o executável atual. O motor deverá poder ser chamado tanto pelo CLI quanto pelos handlers HTTP.

Como o servidor pode receber requisições em paralelo, as operações que alteram o estado da partida — principalmente o sorteio — deverão ser protegidas contra acesso concorrente.

### 3.2 Frontend

- **HTML5:** estrutura da página.
- **CSS3:** layout, identidade visual, cartelas e transições.
- **JavaScript puro:** estado de apresentação, chamadas HTTP e controle da sequência visual.
- **SVG:** ilustração da máquina com elementos independentes.
- **Vite:** servidor de desenvolvimento, proxy da API e build dos arquivos estáticos.

React, Vue, Three.js, Canvas, WebGL e bibliotecas pesadas de animação não são necessários para o escopo inicial.

### 3.3 Desenvolvimento e execução

Durante o desenvolvimento:
- o Vite serve o frontend;
- o proxy do Vite encaminha `/api` ao servidor C;
- frontend e backend podem ser iniciados separadamente.

Na execução de produção:
- o Vite gera os arquivos estáticos em `frontend/dist`;
- o servidor CivetWeb disponibiliza esses arquivos e a API;
- navegador e API podem operar na mesma origem, sem exigir configuração de CORS.

A porta local deverá ser configurável e documentada.

## 4. Contrato inicial da API

Os contratos abaixo são a proposta de referência para a implementação. Podem ser refinados durante a Issue da API, desde que o C continue sendo a fonte de verdade.

| Método | Endpoint | Responsabilidade |
|---|---|---|
| GET | `/api/health` | Verificar se o servidor está ativo |
| POST | `/api/games` | Criar partida e gerar as cartelas |
| GET | `/api/games/current` | Consultar o estado da partida atual |
| POST | `/api/games/current/draw` | Realizar exatamente um sorteio |
| POST | `/api/games/current/reset` | Limpar/encerrar a partida atual |

### 4.1 Criar partida

Requisição ilustrativa:

```json
{
  "players": ["Jogador 1", "Jogador 2", "Jogador 3"]
}
```

A quantidade e os nomes serão validados pelo backend. A resposta deve trazer o estado inicial e as cartelas que o C acabou de gerar.

### 4.2 Estado da partida

A resposta de consulta deve incluir, no mínimo:
- identificador/situação da partida;
- rodada atual;
- jogadores, IDs e nomes;
- para cada jogador, os 15 números da cartela e o estado de marcação de cada posição;
- lista de números sorteados;
- vencedores e respectivas rodadas de vitória;
- indicador de encerramento.

Formato conceitual de uma cartela:

```json
{
  "id": 1,
  "name": "Jogador 1",
  "numbers": [/* 15 números gerados pelo C */],
  "marked": [/* 15 valores booleanos */],
  "won": false,
  "winRound": null
}
```

Os comentários acima são explicativos: a resposta JSON real conterá arrays de valores, não comentários.

### 4.3 Sortear

A chamada `POST /api/games/current/draw` não recebe um número escolhido pelo navegador. O servidor executa o sorteio e devolve, no mínimo:
- número sorteado;
- rodada;
- histórico atualizado;
- cartelas com marcações atualizadas;
- novos vencedores;
- indicador de partida encerrada.

A API deve impedir sorteios quando não houver partida ativa, quando a partida já tiver terminado ou quando os 60 números tiverem sido utilizados.

### 4.4 Erros

As respostas de erro devem usar JSON consistente, por exemplo:

```json
{
  "error": {
    "code": "GAME_FINISHED",
    "message": "A partida já foi encerrada."
  }
}
```

Códigos HTTP sugeridos:
- `400`: entrada inválida;
- `404`: partida inexistente;
- `409`: conflito, partida encerrada ou sorteio indisponível;
- `500`: falha interna.

## 5. Sincronização da animação com o sorteio real

A animação não pode criar um número fictício para dar a impressão de sorteio.

Fluxo recomendado:
1. O clique desabilita temporariamente o controle para evitar chamadas duplicadas.
2. O frontend envia uma única requisição para `POST /api/games/current/draw`.
3. A parte inicial da animação (alavanca e giro do globo) pode começar enquanto a requisição está em andamento.
4. A abertura da portinha e a saída da bola aguardam o resultado real do backend.
5. A bola recebe e revela o número retornado pelo C.
6. As cartelas, o histórico, a rodada e os vencedores são atualizados com o estado retornado pela API.
7. O controle é reabilitado quando a sequência terminar e a interface estiver consistente.

Se a API falhar, a interface não deve revelar número algum. Deve encerrar ou retornar a máquina ao estado de repouso e exibir uma mensagem de erro.

A interface também deverá respeitar a preferência de movimento reduzido do sistema (`prefers-reduced-motion`), permitindo uma transição curta ou revelação direta sem impedir a conclusão da rodada.

## 6. Organização de arquivos sugerida

A estrutura abaixo é uma proposta para a implementação; não deve ser criada integralmente antes das respectivas etapas.

```text
BinGoat/
├── backend/
│   ├── include/
│   │   └── game.h
│   ├── src/
│   │   ├── main.c
│   │   ├── game.c
│   │   ├── game_api.c
│   │   └── server.c
│   └── lib/
│       ├── civetweb/
│       └── cJSON/
├── frontend/
│   ├── index.html
│   ├── src/
│   │   ├── main.js
│   │   ├── api.js
│   │   ├── cards.js
│   │   ├── machine.js
│   │   └── style.css
│   └── assets/
│       └── bingo-machine.svg
├── docs/
│   └── WEB-APP-ROADMAP.md
├── README.md
└── .gitignore
```

A divisão final dos arquivos poderá ser ajustada para evitar fragmentação desnecessária. O objetivo é separar responsabilidades, não criar arquivos por criar.

## 7. Backlog de desenvolvimento

As Issues foram abertas separadamente e categorizadas pelo prefixo do título. Todas estão abertas e usam o rótulo geral `enhancement`.

### Backend e API

| Issue | Entrega | Dependência |
|---|---|---|
| [#3 — Separar o motor do bingo da interface de terminal](https://github.com/bpoltosi/BinGoat/issues/3) | Motor C reutilizável, preservando o CLI | — |
| [#4 — Criar servidor HTTP em C com CivetWeb e cJSON](https://github.com/bpoltosi/BinGoat/issues/4) | Servidor HTTP, health check e entrega de arquivos estáticos | #3 |
| [#5 — Expor criação, consulta, sorteio e reinício de partidas](https://github.com/bpoltosi/BinGoat/issues/5) | Endpoints JSON para o ciclo de vida da partida | #3, #4 |

### Frontend e cartelas

| Issue | Entrega | Dependência |
|---|---|---|
| [#6 — Criar estrutura web e layout responsivo](https://github.com/bpoltosi/BinGoat/issues/6) | Estrutura HTML/CSS/JS com Vite | — |
| [#7 — Renderizar cartelas dinâmicas](https://github.com/bpoltosi/BinGoat/issues/7) | Cartelas 3 × 5 com números vindos da API | #5, #6 |

### Visual e animações

| Issue | Entrega | Dependência |
|---|---|---|
| [#8 — Desenhar a máquina de bingo 2D em SVG](https://github.com/bpoltosi/BinGoat/issues/8) | Ilustração vetorial separada em elementos | #6 |
| [#9 — Implementar a sequência 2D da alavanca, globo e bola](https://github.com/bpoltosi/BinGoat/issues/9) | Animação coordenada e revelação do número | #8 |

### Integração e validação

| Issue | Entrega | Dependência |
|---|---|---|
| [#10 — Conectar a interface à API e sincronizar o sorteio](https://github.com/bpoltosi/BinGoat/issues/10) | Fluxo completo entre navegador e backend | #4, #5, #7, #9 |
| [#11 — Validar o fluxo completo da versão web e a compatibilidade](https://github.com/bpoltosi/BinGoat/issues/11) | Validação funcional, Windows/MinGW e documentação | #10 |

As etapas de frontend inicial (#6) e backend (#3–#5) podem avançar em paralelo depois que o contrato entre as camadas estiver acordado. A integração completa deve ocorrer depois que API, cartelas e animação estiverem disponíveis.

## 8. Critérios de aceite da versão web

A primeira versão será considerada pronta quando:
- [ ] O usuário conseguir criar uma partida com 2 a 5 jogadores pelo navegador.
- [ ] Todas as cartelas forem geradas pelo backend C e exibidas com os números corretos.
- [ ] Os números de cada cartela permanecerem estáveis durante a partida.
- [ ] O usuário conseguir solicitar uma rodada pela interface.
- [ ] A máquina executar a sequência 2D da alavanca, globo, portinha e bola.
- [ ] O número exibido na bola for exatamente o número retornado pela API.
- [ ] As casas correspondentes forem marcadas após o sorteio.
- [ ] Vitórias, rodada de vitória e encerramento forem exibidos corretamente.
- [ ] A interface tratar erros e impedir sorteios duplicados.
- [ ] O projeto puder ser compilado e executado localmente com as instruções documentadas.
- [ ] O README explicar como executar backend, frontend e versão de produção.

## 9. Fora de escopo nesta primeira versão

- Persistência em banco de dados.
- Cadastro de usuários, autenticação ou salas online.
- Várias partidas simultâneas.
- Sincronização entre vários navegadores/dispositivos.
- Efeitos 3D, física realista ou simulação de bolas.
- Áudio, efeitos sonoros ou narração.
- Mudança das regras atuais para bingo 5 × 5 ou 75 bolas.

Esses itens poderão ser avaliados em Issues futuras, depois que a versão web básica estiver funcional.

## 10. Diretriz de implementação

Implementar de forma incremental, mantendo o jogo de terminal funcionando até que a nova interface esteja validada. Cada Issue deve ser desenvolvida e revisada dentro do próprio escopo. Não realizar merge automático de PRs sem revisão.

**Este documento registra a arquitetura e o backlog propostos; não representa uma implementação concluída.**
