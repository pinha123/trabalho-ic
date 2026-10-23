#include "leitorDeInstancias/leitorDeInstancias.h"
#include "metaheuristica/metaheuristica.h"

int main(int argc, char* argv[]) {
    std::string caminho = (argc > 1) ? argv[1] : "instancia_gerada.txt";

    Instancia instancia;
    if (!lerInstancia(caminho, instancia)) {
        return 1;
    }

    std::vector<Grupo> grupos = criarGruposIniciais(instancia.candidatos, instancia.num_escolas);

    agruparCandidatosKNN(instancia.candidatos, grupos);

    alocarGruposEscolas(grupos, instancia.escolas, instancia.candidatos);

    return 0;
}
