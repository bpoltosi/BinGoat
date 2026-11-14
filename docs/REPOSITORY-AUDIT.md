# Auditoria do repositório BinGoat

Data: 2026-11-13  
Branch auditada: `main`  
Commit-base: `f8959986ede203c7b05375ce8bbadef995c2e13a`

## Resultado

A estrutura atual é coerente com a arquitetura C + frontend Vite. Não foram encontrados diretórios de build, binários gerados ou dependências vendorizadas versionadas na árvore principal. Os testes e workflows cobrem o motor C, servidor HTTP, contrato da API e proxy do Vite.

## Decisões sobre arquivos suspeitos

| Item | Decisão | Justificativa |
|---|---|---|
| `TrabalhoBingo.c` | Manter | É o CLI legado preservado e é incluído pelos testes de caracterização em `tests/test_engine.c`. |
| `bingo_engine.c/.h` | Manter | Motor C compartilhado pelo CLI e pelo servidor HTTP. |
| `backend/*` | Manter | Implementação ativa da API e do servidor HTTP nativo. |
| `src/*` | Manter | Implementação ativa do frontend. |
| `tests/*` | Manter | Suítes C, Python, shell e PowerShell usadas localmente e na CI. |
| `docs/WEB-APP-ROADMAP.md` | Manter como histórico | O documento descrevia arquitetura e backlog já superados; foi identificado explicitamente como histórico. |
| `bun.lock` | Removido | Lockfile sem integração com scripts, documentação ou workflows; o projeto usa npm. Vite foi fixado na versão 6.4.6, que era a versão resolvida nesse lockfile. |

## Limpeza aplicada

- Corrigidas mensagens e comentários do frontend que ainda atribuíam a execução do jogo ao arquivo CLI ou descreviam endpoints como futuros.
- Atualizada a identificação do painel de cartelas para referenciar o motor BinGoat.
- Identificado o roadmap inicial como documento histórico, evitando que instruções antigas sejam confundidas com a arquitetura vigente.
- Padronizado o Vite em versão exata e removido o lockfile Bun órfão.
- Corrigido o teste de contrato que usava números de uma geração anterior de cartelas após um reset com regeneração. O teste agora consulta e valida as cartelas correntes antes da marcação manual.

## Pontos para acompanhamento

- O projeto ainda não possui `package-lock.json`; a instalação npm não fica totalmente reproduzível em nível transitivo. Gerar e versionar um lockfile npm deve ser uma tarefa dedicada quando o ambiente de desenvolvimento puder executar npm.
- O roadmap preservado contém descrições antigas de SVG, frontend e endpoints. Ele foi sinalizado como histórico; uma migração futura pode arquivá-lo em uma pasta `docs/archive/` se o histórico não precisar ficar na pasta principal.
- A validação visual manual em desktop/mobile e a revisão de acessibilidade do Canvas não podem ser comprovadas apenas pela leitura estática do repositório.
- A CI mais recente tinha Linux aprovado e Windows falhando no teste de marcação manual. A correção deste PR deve ser confirmada por uma nova execução dos workflows.

## Como validar

```sh
sh tests/run-tests.sh
sh tests/run-server-tests.sh
sh tests/run-vite-proxy-test.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
npm install
npm run build
```

No Windows/MinGW, executar também `./tests/run-tests.ps1` e `./tests/run-server-tests.ps1`.
