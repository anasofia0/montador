# Montador
Trabalho 1 para a disciplina de Software Básico. Consiste na implementação em C/C++ de um montador e ligador do assembly inventado visto em sala de aula.

## Alunos
- Ana Sofia Schweizer Silvestre - 200014382
- Harisson Freitas Magalhães - 202006466

## Sistema Operacional Utilizado
- Linux

## Compilação do Programa
Para compilar o programa, utilize o `Makefile` fornecido. Certifique-se de estar no diretório onde o `Makefile`, `montador.cpp`, e `ligador.cpp` estão localizados.
- 'make': Compila o montador e o ligador. O preprocessador já está no montador.
- 'make clean': Para apagar os arquivos compilados.

## Execução

### Pré-processador e Montador
1. Para gerar um arquivo pré-processado (.pre) a partir de um arquivo de montagem (.asm):
    ./montador -p seu_arquivo.asm

2. Para gerar um arquivo objeto (.obj) a partir de um arquivo pré-processado (.pre):
    ./montador -o seu_arquivo.pre

### Ligador
1. Para ligar múltiplos arquivos objeto (.obj) e gerar um arquivo executável (.e):
    ./ligador arquivo1.obj arquivo2.obj


    O resultado será um arquivo chamado `arquivo1.e`.
