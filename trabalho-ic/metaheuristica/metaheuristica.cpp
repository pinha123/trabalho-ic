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

// Verificação exata de viabilidade: existe alguma alocação em que toda escola respeita
// a capacidade e recebe no máximo maxTipos tipos de prova? Não depende do KNN.

// maxTipos = 1: cada escola atende um tipo só. Cobre um tipo por vez com escolas ainda
// livres ('usadas' é uma máscara de bits); 'sobra' é quanto ainda pode ser desperdiçado.
static bool cobrirTipo(const std::vector<int>& demandas, const std::vector<int>& capacidades,
                       int tipo, long long usadas, int sobra, std::set<std::pair<int, long long>>& falhas);

static bool escolherEscolasDoTipo(const std::vector<int>& demandas, const std::vector<int>& capacidades,
                                  int tipo, int escola, long long usadas, int soma, int sobra,
                                  std::set<std::pair<int, long long>>& falhas) {
    if (soma >= demandas[tipo]) {
        return cobrirTipo(demandas, capacidades, tipo + 1, usadas, sobra - (soma - demandas[tipo]), falhas);
    }
    for (int e = escola; e < (int)capacidades.size(); ++e) {
        if (usadas & (1LL << e)) continue;
        if (soma + capacidades[e] - demandas[tipo] > sobra) continue;
        if (escolherEscolasDoTipo(demandas, capacidades, tipo, e + 1, usadas | (1LL << e), soma + capacidades[e], sobra, falhas)) {
            return true;
        }
    }
    return false;
}

static bool cobrirTipo(const std::vector<int>& demandas, const std::vector<int>& capacidades,
                       int tipo, long long usadas, int sobra, std::set<std::pair<int, long long>>& falhas) {
    if (tipo == (int)demandas.size()) return true;
    if (falhas.count({tipo, usadas})) return false;

    if (escolherEscolasDoTipo(demandas, capacidades, tipo, 0, usadas, 0, sobra, falhas)) return true;
    falhas.insert({tipo, usadas});
    return false;
}

// maxTipos >= 2: escolhe, escola por escola, quais tipos ela aceita. 'cabe[B]' guarda quantas
// vagas ainda podem ir para escolas que só aceitam tipos do conjunto B (bits = tipos) sem deixar
// candidato de fora; começa em demanda(B) + folga (condição de Hall).
static bool escolherTiposDasEscolas(const std::vector<int>& capacidades, const std::vector<int>& conjuntos,
                                    int escola, std::vector<int>& cabe, std::set<std::vector<int>>& falhas) {
    if (escola == (int)capacidades.size()) return true;

    std::vector<int> chave = cabe;
    chave.push_back(escola);
    if (falhas.count(chave)) return false;

    int todos = (int)cabe.size() - 1;
    for (int S : conjuntos) {
        bool cabeEmTodos = true;
        for (int B = 0; B < todos; ++B) {
            if ((B & S) == S && cabe[B] < capacidades[escola]) {
                cabeEmTodos = false;
                break;
            }
        }
        if (!cabeEmTodos) continue;

        for (int B = 0; B < todos; ++B) {
            if ((B & S) == S) cabe[B] -= capacidades[escola];
        }
        bool deuCerto = escolherTiposDasEscolas(capacidades, conjuntos, escola + 1, cabe, falhas);
        for (int B = 0; B < todos; ++B) {
            if ((B & S) == S) cabe[B] += capacidades[escola];
        }
        if (deuCerto) return true;
    }

    falhas.insert(chave);
    return false;
}

bool instanciaViavel(const std::vector<Candidato>& candidatos, const std::vector<Escola>& escolas, int maxTipos) {
    std::map<std::string, int> contagem;
    for (const Candidato& c : candidatos) {
        contagem[c.tipo_prova]++;
    }
    std::vector<int> demandas;
    for (const auto& t : contagem) {
        demandas.push_back(t.second);
    }

    std::vector<int> capacidades;
    for (const Escola& e : escolas) {
        capacidades.push_back(e.capacidadeMaxima);
    }
    std::sort(capacidades.begin(), capacidades.end(), std::greater<int>());

    int folga = 0;
    for (int c : capacidades) folga += c;
    for (int d : demandas) folga -= d;

    int numTipos = demandas.size();
    if (folga < 0) return false;
    if (numTipos == 0) return true;
    if (maxTipos <= 0) return false;
    if (maxTipos >= numTipos) return true;

    if (maxTipos == 1) {
        std::sort(demandas.begin(), demandas.end(), std::greater<int>());
        std::set<std::pair<int, long long>> falhas;
        return cobrirTipo(demandas, capacidades, 0, 0, folga, falhas);
    }

    std::vector<int> conjuntos;
    for (int S = 0; S < (1 << numTipos); ++S) {
        if (__builtin_popcount(S) == maxTipos) conjuntos.push_back(S);
    }

    std::vector<int> cabe(1 << numTipos, folga);
    for (int B = 0; B < (1 << numTipos); ++B) {
        for (int t = 0; t < numTipos; ++t) {
            if (B & (1 << t)) cabe[B] += demandas[t];
        }
    }

    std::set<std::vector<int>> falhas;
    return escolherTiposDasEscolas(capacidades, conjuntos, 0, cabe, falhas);
}

int criarSolucaoInicial(std::vector<Candidato>& candidatos, std::vector<Escola>& escolas) {
    if (!instanciaViavel(candidatos, escolas, MAX_TIPOS_POR_ESCOLA)) {
        return -1;
    }

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
