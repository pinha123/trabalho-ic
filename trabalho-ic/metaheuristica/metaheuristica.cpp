#include "metaheuristica.h"

#include <cmath>
#include <limits>

int K_VIZINHOS = 1;
int PESO_TIPO_PROVA = 2;

double calcularDistancia(double lat1, double lon1, double lat2, double lon2, std::string tipo_prova1, std::string tipo_prova2) {
    double distanciaEuclidiana = std::sqrt(std::pow(lat1 - lat2, 2) + std::pow(lon1 - lon2, 2));
    int distanciaTipoProva = (tipo_prova1 != tipo_prova2) ? PESO_TIPO_PROVA : 0;
    double distanciaTotal = std::sqrt(std::pow(distanciaEuclidiana, 2) + std::pow(distanciaTipoProva, 2));
    return distanciaTotal;
}

std::vector<Grupo> criarGruposIniciais(const std::vector<Candidato>& candidatos, int num_grupos) {
    std::vector<Grupo> grupos;
    for (int i = 0; i < num_grupos && i < candidatos.size(); ++i) {
        Grupo g;
        g.id = i;
        g.latitude = candidatos[i].latitude;
        g.longitude = candidatos[i].longitude;
        g.tipo_prova = candidatos[i].tipo_prova;
        grupos.push_back(g);
    }
    return grupos;
}

void agruparCandidatosKNN(std::vector<Candidato>& candidatos, std::vector<Grupo>& grupos) {
    for (size_t i = 0; i < candidatos.size(); ++i) {
        double menor_distancia = std::numeric_limits<double>::max();
        int id_melhor_grupo = -1;

        for (size_t j = 0; j < grupos.size(); ++j) {
            double dist = calcularDistancia(
                candidatos[i].latitude, candidatos[i].longitude,
                grupos[j].latitude, grupos[j].longitude,
                candidatos[i].tipo_prova, grupos[j].tipo_prova
            );

            if (dist < menor_distancia) {
                menor_distancia = dist;
                id_melhor_grupo = j;
            }
        }

        if (id_melhor_grupo != -1) {
            grupos[id_melhor_grupo].candidatos_alocados.push_back(i);
        }
    }
}

void alocarGruposEscolas(std::vector<Grupo>& grupos, std::vector<Escola>& escolas, std::vector<Candidato>& candidatos) {
        for (size_t i = 0; i < grupos.size(); ++i) {
        double menor_dist = std::numeric_limits<double>::max();
        int id_escola_proxima = -1;

        for (size_t j = 0; j < escolas.size(); ++j) {
            double dist = std::sqrt(std::pow(grupos[i].latitude - escolas[j].latitude, 2) + std::pow(grupos[i].longitude - escolas[j].longitude, 2));
            if (dist < menor_dist) {
                menor_dist = dist;
                id_escola_proxima = j;
            }
        }

        if (id_escola_proxima != -1) {
            for (int idx_cand : grupos[i].candidatos_alocados) {
                candidatos[idx_cand].id_escola_atribuida = escolas[id_escola_proxima].id;
                escolas[id_escola_proxima].candidatos_alocados.push_back(idx_cand);
            }
        }
    }
}
