#ifndef LEITOR_DE_INSTANCIAS_H
#define LEITOR_DE_INSTANCIAS_H

#include <string>
#include <vector>

struct Candidato {
    std::string id;
    std::string cep;
    std::string tipo_prova;
    double latitude;
    double longitude;
    int id_escola_atribuida = -1;
};

struct Escola {
    int id;                              // "E7" -> 7
    std::string cep;
    double latitude;
    double longitude;
    std::vector<int> capacidades_salas;  // uma entrada por sala
    std::vector<int> candidatos_alocados;
    int capacidadeMaxima = 0;            // soma das capacidades das salas
    bool estaCheia = false;
    int quantidadeTipoProvas = 0;
};

struct Instancia {
    int num_candidatos = 0;
    int num_escolas = 0;
    std::vector<Candidato> candidatos;
    std::vector<Escola> escolas;
    // matriz_distancias[i][j] = distância em km do candidato i à escola j (mesma ordem do arquivo)
    std::vector<std::vector<double>> matriz_distancias;
};

// Lê uma instância no formato de instancia_gerada.txt.
// Em caso de erro, imprime arquivo:linha e o motivo em std::cerr e retorna false.
bool lerInstancia(const std::string& caminho, Instancia& instancia);

#endif
