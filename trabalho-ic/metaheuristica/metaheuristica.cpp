#include "metaheuristica.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <set>

int K_VIZINHOS = 1;
int PESO_TIPO_PROVA = 2;
int MAX_TIPOS_POR_ESCOLA = 3;

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

static std::map<std::string, int> contarTipos(const Escola& escola, const std::vector<Candidato>& candidatos) {
    std::map<std::string, int> tipos;
    for (int idx_cand : escola.candidatos_alocados) {
        tipos[candidatos[idx_cand].tipo_prova]++;
    }
    return tipos;
}

bool validarSolucao(const std::vector<Candidato>& candidatos, const std::vector<Escola>& escolas) {
    for (const Candidato& c : candidatos) {
        if (c.id_escola_atribuida == -1) {
            return false;
        }
    }

    for (const Escola& e : escolas) {
        if ((int)e.candidatos_alocados.size() > e.capacidadeMaxima) {
            return false;
        }
        if ((int)contarTipos(e, candidatos).size() > MAX_TIPOS_POR_ESCOLA) {
            return false;
        }
    }
    return true;
}

static int acharEscolaComVaga(int origem, const std::string& tipo_prova, const std::vector<Escola>& escolas, const std::vector<Candidato>& candidatos) {
    for (size_t j = 0; j < escolas.size(); ++j) {
        if ((int)j == origem) continue;
        if ((int)escolas[j].candidatos_alocados.size() >= escolas[j].capacidadeMaxima) continue;

        std::map<std::string, int> tipos = contarTipos(escolas[j], candidatos);
        if (tipos.count(tipo_prova) > 0 || (int)tipos.size() < MAX_TIPOS_POR_ESCOLA) {
            return j;
        }
    }
    return -1;
}

int criarSolucaoInicial(std::vector<Candidato>& candidatos, std::vector<Escola>& escolas) {
    std::vector<Grupo> grupos = criarGruposIniciais(candidatos, escolas.size());
    agruparCandidatosKNN(candidatos, grupos);
    alocarGruposEscolas(grupos, escolas, candidatos);

    if (validarSolucao(candidatos, escolas)) {
        return 0;
    }

    for (size_t i = 0; i < escolas.size(); ++i) {
        std::map<std::string, int> tipos = contarTipos(escolas[i], candidatos);
        std::vector<std::pair<int, std::string>> tipos_por_quantidade;
        for (const auto& t : tipos) {
            tipos_por_quantidade.push_back({t.second, t.first});
        }
        std::sort(tipos_por_quantidade.begin(), tipos_por_quantidade.end(), std::greater<std::pair<int, std::string>>());

        std::set<std::string> tipos_mantidos;
        for (int k = 0; k < (int)tipos_por_quantidade.size() && k < MAX_TIPOS_POR_ESCOLA; ++k) {
            tipos_mantidos.insert(tipos_por_quantidade[k].second);
        }


        std::vector<int> ficam;
        std::vector<int> excedentes;
        for (int idx_cand : escolas[i].candidatos_alocados) {
            bool tipo_ok = tipos_mantidos.count(candidatos[idx_cand].tipo_prova) > 0;
            if (tipo_ok && (int)ficam.size() < escolas[i].capacidadeMaxima) {
                ficam.push_back(idx_cand);
            } else {
                excedentes.push_back(idx_cand);
            }
        }
        escolas[i].candidatos_alocados = ficam;

        for (int idx_cand : excedentes) {
            int destino = acharEscolaComVaga(i, candidatos[idx_cand].tipo_prova, escolas, candidatos);
            if (destino == -1) {
                return -1;
            }
            escolas[destino].candidatos_alocados.push_back(idx_cand);
            candidatos[idx_cand].id_escola_atribuida = escolas[destino].id;
        }
    }

    return validarSolucao(candidatos, escolas) ? 0 : -1;
}
