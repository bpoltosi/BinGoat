# Testes de caracterização do motor C

Esta suíte registra o comportamento que já existe em `TrabalhoBingo.c` antes da extração do motor. Os testes incluem o arquivo original diretamente, renomeando apenas o `main` durante a compilação de teste. Nenhuma regra ou função de produção é alterada para facilitar os testes.

## Cobertura

- Aceita partidas com 2, 3, 4 ou 5 jogadores e solicita novamente entradas abaixo/acima do intervalo, não numéricas ou com caracteres extras.
- Gera cartelas com 15 números em ordem crescente, distintos, entre 1 e 60, sem números consecutivos e com todas as marcações inicialmente limpas.
- Confirma que a mesma semente reproduz a sequência de uma cartela e de sorteios no mesmo runtime C.
- Percorre os 60 números sorteáveis, verificando intervalo, ausência de repetição e bloqueio de um 61º sorteio.
- Caracteriza a marcação do programa de terminal, que não altera a cartela de um jogador já declarado vencedor.
- Verifica a vitória somente quando os 15 números estão marcados e preserva a rodada da primeira vitória.
- Executa o fluxo real de terminal com dois jogadores e confirma que ambos vencem e que o ranking final é exibido dentro do limite de 60 rodadas.
- Verifica a detecção de linhas horizontais, verticais e das duas orientações diagonais.
- Executa os comandos web atuais 6 e 7 em cenários fixos: desfazer o último sorteio (incluindo a sincronização de marcações/vencedor) e reiniciar sem regenerar as cartelas.

## Executar no Linux ou macOS

Requer GCC compatível com C11:

```bash
sh tests/run-tests.sh
```

## Executar no Windows com MinGW-w64

Instale o GCC do MinGW-w64 e confirme que `gcc` está disponível no `PATH`. Na raiz do repositório, execute no PowerShell:

```powershell
.\tests\run-tests.ps1
```

Se a política de execução do PowerShell bloquear scripts locais:

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run-tests.ps1
```

O compilador pode ser selecionado pela variável de ambiente `CC`, se necessário.

## Reprodutibilidade

Os testes usam sementes fixas por meio de `srand()` e comparam execuções repetidas dentro do mesmo runtime. A biblioteca C não garante que `rand()` produza a mesma sequência entre implementações diferentes; portanto, a suíte não fixa números específicos que poderiam variar entre GCC/glibc e MinGW-w64. Os cenários são reproduzíveis em cada ambiente sem depender de resultados fortuitos.

## Dependências e artefatos

A suíte não inicia o Node.js, o Vite ou o frontend e não requer npm. Os executáveis de teste são criados em um diretório temporário do sistema; arquivos temporários de entrada/saída são removidos ao final e estão ignorados pelo Git.

O workflow `.github/workflows/c-engine-tests.yml` executa a mesma cobertura em Linux/GCC e Windows/MinGW-w64 a cada pull request que altera o motor ou os testes.
