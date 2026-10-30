#ifndef METAHEURISTICA_H
#define METAHEURISTICA_H

#include <map>
#include <string>
#include <vector>

#include "../leitorDeInstancias/leitorDeInstancias.h"

extern int PESO_TIPO_PROVA;
extern int MAX_TIPOS_POR_ESCOLA;
extern int MAX_ITERACOES_KMEANS;

struct Grupo {
    int id;
    double latitude;
    double longitude;
    std::map<std::string, double> proporcao_tipos;
    std::vector<int> candidatos_alocados;
};

double distanciaKm(double lat1, double lon1, double lat2, double lon2);
double calcularDistancia(const Candidato& candidato, const Grupo& grupo);
std::vector<Grupo> criarGruposIniciais(const std::vector<Candidato>& candidatos, int num_grupos);
void agruparCandidatosKMeans(std::vector<Candidato>& candidatos, std::vector<Grupo>& grupos);
void alocarGruposEscolas(std::vector<Grupo>& grupos, std::vector<Escola>& escolas, std::vector<Candidato>& candidatos);


// Cada sala recebe um único tipo de prova: diz se as salas podem ser divididas entre os tipos
// de forma que cada tipo tenha lugar para as suas quantidades de candidatos.
bool salasComportam(const std::vector<int>& capacidades_salas, const std::vector<int>& quantidades);

bool validarSolucao(const std::vector<Candidato>& candidatos, const std::vector<Escola>& escolas);

bool instanciaViavel(const std::vector<Candidato>& candidatos, const std::vector<Escola>& escolas, int maxTipos);


int criarSolucaoInicial(std::vector<Candidato>& candidatos, std::vector<Escola>& escolas);

double avaliarSolucao(const Instancia& instancia);

bool moverAluno(Instancia& instancia);
bool trocarDoisAlunos(Instancia& instancia);
bool trocarTresAlunos(Instancia& instancia);

int executarVND(Instancia& instancia);

#endif
