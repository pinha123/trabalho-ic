#!/usr/bin/env bash
# Compila e executa o projeto.
# Uso: ./executar.sh [arquivo_lista_de_instancias]
# Os caminhos em Instancias/Instancias.txt são relativos a trabalho-ic/, então o programa roda de lá

set -euo pipefail

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FONTES="$RAIZ/trabalho-ic"
BINARIO="$FONTES/programa"

cd "$FONTES"

echo "Compilando..."
g++ -std=c++17 -O2 -Wall -Wextra \
    main.cpp \
    leitorDeInstancias/leitorDeInstancias.cpp \
    metaheuristica/metaheuristica.cpp \
    -o "$BINARIO"

echo "Executando..."
"$BINARIO" "$@"
