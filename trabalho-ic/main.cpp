#include "leitorDeInstancias/leitorDeInstancias.h"
#include "metaheuristica/metaheuristica.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
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

static void imprimirTiposPorEscola(const Instancia& instancia) {
    for (const Escola& escola : instancia.escolas) {
        std::map<std::string, int> contagem;
        for (int indiceCandidato : escola.candidatos_alocados) {
            ++contagem[instancia.candidatos[indiceCandidato].tipo_prova];
        }

        std::cerr << "Escola " << escola.id
                  << ": demanda=" << escola.candidatos_alocados.size()
                  << ", capacidade=" << escola.capacidadeMaxima << ":";
        if (contagem.empty()) {
            std::cerr << " nenhuma prova";
        } else {
            for (const auto& item : contagem) {
                std::cerr << ' ' << item.first << '=' << item.second;
            }
        }
        std::cerr << std::endl;
    }
}

static int contarAlunosNaoAlocados(const Instancia& instancia) {
    int quantidade = 0;
    for (const Candidato& candidato : instancia.candidatos) {
        if (candidato.id_escola_atribuida == -1) {
            ++quantidade;
        }
    }
    return quantidade;
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
    constexpr int NUM_EXECUCOES = 1;

    std::vector<Instancia> instancias;
    if (!lerTodasInstancias(caminhoArquivoLista, instancias)) {
        return 1;
    }

    std::cout << "Nome_instancia,custo_avg,custo_best,tempo_medio,custo_inicial"
              << std::endl;
    std::cout << std::fixed << std::setprecision(6);

    for (const Instancia& instancia : instancias) {
        double somaCustos = 0.0;
        double melhorCusto = std::numeric_limits<double>::max();
        double somaTempos = 0.0;
        double custoInicial = 0.0;

        for (int execucao = 0; execucao < NUM_EXECUCOES; ++execucao) {
            Instancia solucaoInicial = instancia;
            if (criarSolucaoInicial(
                    solucaoInicial.candidatos,
                    solucaoInicial.escolas
                ) != 0) {
                std::cerr << "Erro: a instância '" << instancia.nome
                          << "' não possui solução viável." << std::endl;
                /* std::cerr << "Alunos não alocados: "
                          << contarAlunosNaoAlocados(solucaoInicial)
                          << std::endl; */
                          break;
                        }
                        
                        //imprimirTiposPorEscola(solucaoInicial);
            if (execucao == 0) {
                custoInicial = avaliarSolucao(solucaoInicial);
            }

            Instancia solucao = instancia;
            const auto inicio = std::chrono::steady_clock::now();
            if (executarVND(solucao) != 0) {
                std::cerr << "Erro: falha ao criar solução para a instância '"
                          << instancia.nome << "'." << std::endl;
                break;
            }
            const auto fim = std::chrono::steady_clock::now();

            const double custo = avaliarSolucao(solucao);
            const double tempo = std::chrono::duration<double, std::milli>(
                fim - inicio
            ).count();
            somaCustos += custo;
            melhorCusto = std::min(melhorCusto, custo);
            somaTempos += tempo;
        }

        if (melhorCusto == std::numeric_limits<double>::max()) {
            continue;
        }

        std::cout << instancia.nome << ','
                  << somaCustos / NUM_EXECUCOES << ','
                  << melhorCusto << ','
                  << somaTempos / NUM_EXECUCOES << ','
                  << custoInicial << std::endl;
    }

    return 0;
}
