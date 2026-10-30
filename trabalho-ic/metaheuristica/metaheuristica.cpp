#include "metaheuristica.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <random>
#include <set>

int PESO_TIPO_PROVA = 2;
int MAX_TIPOS_POR_ESCOLA = 3;
int MAX_ITERACOES_KMEANS = 100;

double distanciaKm(double lat1, double lon1, double lat2, double lon2) {
    const double KM_POR_GRAU = 111.32;
    const double PI = 3.14159265358979;
    double dLat = (lat1 - lat2) * KM_POR_GRAU;
    double dLon = (lon1 - lon2) * KM_POR_GRAU * std::cos((lat1 + lat2) / 2 * PI / 180);
    return std::sqrt(dLat * dLat + dLon * dLon);
}

double calcularDistancia(const Candidato& candidato, const Grupo& grupo) {
    double geografica = distanciaKm(candidato.latitude, candidato.longitude, grupo.latitude, grupo.longitude);

    double diferencaTipo = 0;
    for (const auto& t : grupo.proporcao_tipos) {
        double indicador = (t.first == candidato.tipo_prova) ? 1.0 : 0.0;
        diferencaTipo += (indicador - t.second) * (indicador - t.second);
    }
    if (grupo.proporcao_tipos.count(candidato.tipo_prova) == 0) {
        diferencaTipo += 1;
    }

    return std::sqrt(geografica * geografica + PESO_TIPO_PROVA * PESO_TIPO_PROVA * diferencaTipo);
}

static Grupo grupoNoCandidato(const Candidato& candidato, int id) {
    Grupo g;
    g.id = id;
    g.latitude = candidato.latitude;
    g.longitude = candidato.longitude;
    g.proporcao_tipos[candidato.tipo_prova] = 1.0;
    return g;
}

std::vector<Grupo> criarGruposIniciais(const std::vector<Candidato>& candidatos, int num_grupos) {
    std::vector<Grupo> grupos;
    if (candidatos.empty() || num_grupos <= 0) {
        return grupos;
    }

    std::mt19937 gerador(42);
    std::uniform_int_distribution<int> sorteio(0, (int)candidatos.size() - 1);
    grupos.push_back(grupoNoCandidato(candidatos[sorteio(gerador)], 0));

    while ((int)grupos.size() < num_grupos) {
        std::vector<double> pesos(candidatos.size());
        double total = 0;
        for (size_t i = 0; i < candidatos.size(); ++i) {
            double menor = std::numeric_limits<double>::max();
            for (const Grupo& g : grupos) {
                menor = std::min(menor, calcularDistancia(candidatos[i], g));
            }
            pesos[i] = menor * menor;
            total += pesos[i];
        }
        if (total == 0) {
            break;
        }

        std::discrete_distribution<int> escolha(pesos.begin(), pesos.end());
        grupos.push_back(grupoNoCandidato(candidatos[escolha(gerador)], grupos.size()));
    }
    return grupos;
}

void agruparCandidatosKMeans(std::vector<Candidato>& candidatos, std::vector<Grupo>& grupos) {
    std::vector<int> grupoDoCandidato(candidatos.size(), -1);

    for (int iteracao = 0; iteracao < MAX_ITERACOES_KMEANS; ++iteracao) {
        bool mudou = false;
        for (size_t i = 0; i < candidatos.size(); ++i) {
            double menor_distancia = std::numeric_limits<double>::max();
            int melhor_grupo = -1;
            for (size_t j = 0; j < grupos.size(); ++j) {
                double dist = calcularDistancia(candidatos[i], grupos[j]);
                if (dist < menor_distancia) {
                    menor_distancia = dist;
                    melhor_grupo = j;
                }
            }
            if (melhor_grupo != grupoDoCandidato[i]) {
                grupoDoCandidato[i] = melhor_grupo;
                mudou = true;
            }
        }
        if (!mudou) {
            break;
        }

        for (size_t j = 0; j < grupos.size(); ++j) {
            double somaLat = 0, somaLon = 0;
            int quantos = 0;
            std::map<std::string, double> contagemTipos;
            for (size_t i = 0; i < candidatos.size(); ++i) {
                if (grupoDoCandidato[i] != (int)j) continue;
                somaLat += candidatos[i].latitude;
                somaLon += candidatos[i].longitude;
                contagemTipos[candidatos[i].tipo_prova] += 1;
                quantos++;
            }
            if (quantos == 0) continue;

            grupos[j].latitude = somaLat / quantos;
            grupos[j].longitude = somaLon / quantos;
            grupos[j].proporcao_tipos.clear();
            for (const auto& t : contagemTipos) {
                grupos[j].proporcao_tipos[t.first] = t.second / quantos;
            }
        }
    }

    for (Grupo& g : grupos) {
        g.candidatos_alocados.clear();
    }
    for (size_t i = 0; i < candidatos.size(); ++i) {
        if (grupoDoCandidato[i] != -1) {
            grupos[grupoDoCandidato[i]].candidatos_alocados.push_back(i);
        }
    }
}

void alocarGruposEscolas(std::vector<Grupo>& grupos, std::vector<Escola>& escolas, std::vector<Candidato>& candidatos) {
        for (size_t i = 0; i < grupos.size(); ++i) {
        double menor_dist = std::numeric_limits<double>::max();
        int id_escola_proxima = -1;

        for (size_t j = 0; j < escolas.size(); ++j) {
            double dist = distanciaKm(grupos[i].latitude, grupos[i].longitude, escolas[j].latitude, escolas[j].longitude);
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

// apenas para validar instâncias, feito por IA
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
        int tiposNoConjunto = 0;
        for (int t = 0; t < numTipos; ++t) {
            if (S & (1 << t)) tiposNoConjunto++;
        }
        if (tiposNoConjunto == maxTipos) conjuntos.push_back(S);
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
// apenas para validar instâncias, feito por IA

int criarSolucaoInicial(std::vector<Candidato>& candidatos, std::vector<Escola>& escolas) {
    if (!instanciaViavel(candidatos, escolas, MAX_TIPOS_POR_ESCOLA)) {
        return -1;
    }

    std::vector<Grupo> grupos = criarGruposIniciais(candidatos, escolas.size());
    agruparCandidatosKMeans(candidatos, grupos);
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

double avaliarSolucao(const Instancia& instancia) {
    double soma = 0;
    for (size_t j = 0; j < instancia.escolas.size(); ++j) {
        for (int idx_cand : instancia.escolas[j].candidatos_alocados) {
            soma += instancia.matriz_distancias[idx_cand][j];
        }
    }
    return soma;
}

struct DadosTroca {
    int numTipos;
    std::vector<int> tipoDoAluno;
    std::vector<std::vector<int>> quantosDoTipo;
    std::vector<std::vector<std::vector<double>>> melhorVariacao;
    std::vector<std::vector<std::vector<int>>> melhorAluno;
};

static DadosTroca montarDadosTroca(const Instancia& instancia) {
    DadosTroca d;
    int numEscolas = instancia.escolas.size();

    std::map<std::string, int> indiceTipo;
    for (const Candidato& c : instancia.candidatos) {
        if (indiceTipo.count(c.tipo_prova) == 0) {
            int novo = indiceTipo.size();
            indiceTipo[c.tipo_prova] = novo;
        }
        d.tipoDoAluno.push_back(indiceTipo[c.tipo_prova]);
    }
    d.numTipos = indiceTipo.size();

    d.quantosDoTipo.assign(numEscolas, std::vector<int>(d.numTipos, 0));
    d.melhorVariacao.assign(numEscolas, std::vector<std::vector<double>>(numEscolas, std::vector<double>(d.numTipos, std::numeric_limits<double>::max())));
    d.melhorAluno.assign(numEscolas, std::vector<std::vector<int>>(numEscolas, std::vector<int>(d.numTipos, -1)));

    for (int de = 0; de < numEscolas; ++de) {
        for (int aluno : instancia.escolas[de].candidatos_alocados) {
            int t = d.tipoDoAluno[aluno];
            d.quantosDoTipo[de][t]++;
            for (int para = 0; para < numEscolas; ++para) {
                if (para == de) continue;
                double variacao = instancia.matriz_distancias[aluno][para] - instancia.matriz_distancias[aluno][de];
                if (variacao < d.melhorVariacao[de][para][t]) {
                    d.melhorVariacao[de][para][t] = variacao;
                    d.melhorAluno[de][para][t] = aluno;
                }
            }
        }
    }
    return d;
}

static bool tiposOk(const DadosTroca& d, int escola, int tipoQueSai, int tipoQueEntra) {
    int tiposDiferentes = 0;
    for (int t = 0; t < d.numTipos; ++t) {
        int quantos = d.quantosDoTipo[escola][t];
        if (t == tipoQueSai) quantos--;
        if (t == tipoQueEntra) quantos++;
        if (quantos > 0) tiposDiferentes++;
    }
    return tiposDiferentes <= MAX_TIPOS_POR_ESCOLA;
}

static void transferirAluno(Instancia& instancia, int aluno, int de, int para) {
    std::vector<int>& lista = instancia.escolas[de].candidatos_alocados;
    lista.erase(std::find(lista.begin(), lista.end(), aluno));
    instancia.escolas[para].candidatos_alocados.push_back(aluno);
    instancia.candidatos[aluno].id_escola_atribuida = instancia.escolas[para].id;
}

bool moverAluno(Instancia& instancia) {
    DadosTroca d = montarDadosTroca(instancia);
    int numEscolas = instancia.escolas.size();

    double melhor = -1e-9;
    int melhorDe = -1, melhorPara = -1, melhorTipo = -1;
    for (int de = 0; de < numEscolas; ++de) {
        for (int para = 0; para < numEscolas; ++para) {
            if (para == de) continue;
            if ((int)instancia.escolas[para].candidatos_alocados.size() >= instancia.escolas[para].capacidadeMaxima) continue;
            for (int t = 0; t < d.numTipos; ++t) {
                if (d.melhorAluno[de][para][t] == -1) continue;
                if (d.melhorVariacao[de][para][t] >= melhor) continue;
                if (!tiposOk(d, para, -1, t)) continue;
                melhor = d.melhorVariacao[de][para][t];
                melhorDe = de;
                melhorPara = para;
                melhorTipo = t;
            }
        }
    }

    if (melhorDe == -1) return false;
    transferirAluno(instancia, d.melhorAluno[melhorDe][melhorPara][melhorTipo], melhorDe, melhorPara);
    return true;
}

bool trocarDoisAlunos(Instancia& instancia) {
    DadosTroca d = montarDadosTroca(instancia);
    int numEscolas = instancia.escolas.size();

    double melhor = -1e-9;
    int melhorA = -1, melhorB = -1, melhorTa = -1, melhorTb = -1;
    for (int a = 0; a < numEscolas; ++a) {
        for (int b = a + 1; b < numEscolas; ++b) {
            for (int ta = 0; ta < d.numTipos; ++ta) {
                if (d.melhorAluno[a][b][ta] == -1) continue;
                for (int tb = 0; tb < d.numTipos; ++tb) {
                    if (d.melhorAluno[b][a][tb] == -1) continue;
                    double variacao = d.melhorVariacao[a][b][ta] + d.melhorVariacao[b][a][tb];
                    if (variacao >= melhor) continue;
                    if (!tiposOk(d, a, ta, tb) || !tiposOk(d, b, tb, ta)) continue;
                    melhor = variacao;
                    melhorA = a;
                    melhorB = b;
                    melhorTa = ta;
                    melhorTb = tb;
                }
            }
        }
    }

    if (melhorA == -1) return false;
    int alunoA = d.melhorAluno[melhorA][melhorB][melhorTa];
    int alunoB = d.melhorAluno[melhorB][melhorA][melhorTb];
    transferirAluno(instancia, alunoA, melhorA, melhorB);
    transferirAluno(instancia, alunoB, melhorB, melhorA);
    return true;
}

bool trocarTresAlunos(Instancia& instancia) {
    DadosTroca d = montarDadosTroca(instancia);
    int numEscolas = instancia.escolas.size();

    double melhor = -1e-9;
    int melhorA = -1, melhorB = -1, melhorC = -1, melhorTa = -1, melhorTb = -1, melhorTc = -1;
    for (int a = 0; a < numEscolas; ++a) {
        for (int b = 0; b < numEscolas; ++b) {
            if (b == a) continue;
            for (int c = 0; c < numEscolas; ++c) {
                if (c == a || c == b) continue;
                for (int ta = 0; ta < d.numTipos; ++ta) {
                    if (d.melhorAluno[a][b][ta] == -1) continue;
                    for (int tb = 0; tb < d.numTipos; ++tb) {
                        if (d.melhorAluno[b][c][tb] == -1) continue;
                        for (int tc = 0; tc < d.numTipos; ++tc) {
                            if (d.melhorAluno[c][a][tc] == -1) continue;
                            double variacao = d.melhorVariacao[a][b][ta] + d.melhorVariacao[b][c][tb] + d.melhorVariacao[c][a][tc];
                            if (variacao >= melhor) continue;
                            if (!tiposOk(d, a, ta, tc) || !tiposOk(d, b, tb, ta) || !tiposOk(d, c, tc, tb)) continue;
                            melhor = variacao;
                            melhorA = a;
                            melhorB = b;
                            melhorC = c;
                            melhorTa = ta;
                            melhorTb = tb;
                            melhorTc = tc;
                        }
                    }
                }
            }
        }
    }

    if (melhorA == -1) return false;
    int alunoA = d.melhorAluno[melhorA][melhorB][melhorTa];
    int alunoB = d.melhorAluno[melhorB][melhorC][melhorTb];
    int alunoC = d.melhorAluno[melhorC][melhorA][melhorTc];
    transferirAluno(instancia, alunoA, melhorA, melhorB);
    transferirAluno(instancia, alunoB, melhorB, melhorC);
    transferirAluno(instancia, alunoC, melhorC, melhorA);
    return true;
}

static bool aplicarVizinhanca(int k, Instancia& instancia) {
    switch (k) {
        case 1: return moverAluno(instancia);
        case 2: return trocarDoisAlunos(instancia);
        case 3: return trocarTresAlunos(instancia);
    }
    return false;
}

int executarVND(Instancia& instancia) {
    int resultado = criarSolucaoInicial(instancia.candidatos, instancia.escolas);
    if (resultado != 0) {
        return resultado;
    }

    const int NUM_VIZINHANCAS = 3;
    int k = 1;
    while (k <= NUM_VIZINHANCAS) {
        if (aplicarVizinhanca(k, instancia)) {
            k = 1;
        } else {
            k++;
        }
    }

    return 0;
}
