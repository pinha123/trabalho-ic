#include "leitorDeInstancias/leitorDeInstancias.h"
#include "metaheuristica/metaheuristica.h"

#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

static std::string aparar(const std::string& texto) {
    const size_t inicio = texto.find_first_not_of(" \t\r");
    if (inicio == std::string::npos) {
        return "";
    }

    const size_t fim = texto.find_last_not_of(" \t\r");
    return texto.substr(inicio, fim - inicio + 1);
}

bool lerTodasInstancias(
    const std::string& caminhoArquivoLista,
    std::vector<Instancia>& instancias
) {
    std::ifstream arquivoLista(caminhoArquivoLista);
    if (!arquivoLista.is_open()) {
        std::cerr << "Erro: não foi possível abrir o arquivo de instâncias '"
                  << caminhoArquivoLista << "'" << std::endl;
        return false;
    }

    instancias.clear();

    std::string linha;
    int numeroLinha = 0;
    while (std::getline(arquivoLista, linha)) {
        ++numeroLinha;
        const std::string caminhoInstancia = aparar(linha);

        if (caminhoInstancia.empty()) {
            continue;
        }

        Instancia instancia;
        if (!lerInstancia(caminhoInstancia, instancia)) {
            std::cerr << "Erro ao ler a instância indicada na linha "
                      << numeroLinha << " de '" << caminhoArquivoLista
                      << "'." << std::endl;
            return false;
        }

        instancias.push_back(std::move(instancia));
    }

    return true;
}

int main(int argc, char* argv[]) {
    const std::string caminhoArquivoLista =
        (argc > 1) ? argv[1] : "../Instancias/Instancias.txt";

    std::vector<Instancia> instancias;
    if (!lerTodasInstancias(caminhoArquivoLista, instancias)) {
        return 1;
    }

    for (Instancia& instancia : instancias) {
        std::cout << "Instância: " << instancia.nome << std::endl;
        std::vector<Grupo> grupos =
            criarGruposIniciais(instancia.candidatos, instancia.num_escolas);

        agruparCandidatosKMeans(instancia.candidatos, grupos);

        alocarGruposEscolas(
            grupos,
            instancia.escolas,
            instancia.candidatos
        );
    }

    return 0;
}
