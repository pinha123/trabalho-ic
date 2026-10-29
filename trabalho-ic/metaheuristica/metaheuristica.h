#ifndef METAHEURISTICA_H
#define METAHEURISTICA_H

#include <string>
#include <vector>

#include "../leitorDeInstancias/leitorDeInstancias.h"

extern int K_VIZINHOS;
extern int PESO_TIPO_PROVA;
extern int MAX_TIPOS_POR_ESCOLA;

struct Grupo {
    int id;
    double latitude;
    double longitude;
    std::string tipo_prova;
    std::vector<int> candidatos_alocados;
};

double calcularDistancia(double lat1, double lon1, double lat2, double lon2, std::string tipo_prova1, std::string tipo_prova2);
std::vector<Grupo> criarGruposIniciais(const std::vector<Candidato>& candidatos, int num_grupos);
void agruparCandidatosKNN(std::vector<Candidato>& candidatos, std::vector<Grupo>& grupos);
void alocarGruposEscolas(std::vector<Grupo>& grupos, std::vector<Escola>& escolas, std::vector<Candidato>& candidatos);


bool validarSolucao(const std::vector<Candidato>& candidatos, const std::vector<Escola>& escolas);

bool instanciaViavel(const std::vector<Candidato>& candidatos, const std::vector<Escola>& escolas, int maxTipos);


int criarSolucaoInicial(std::vector<Candidato>& candidatos, std::vector<Escola>& escolas);

#endif
