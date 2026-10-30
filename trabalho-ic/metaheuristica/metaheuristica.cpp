#include "metaheuristica.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <queue>

int K_VIZINHOS = 1;
int PESO_TIPO_PROVA = 2;
int MAX_TIPOS_POR_ESCOLA = 2;

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
                                    int escola, std::vector<int>& cabe, std::set<std::vector<int>>& falhas,
                                    std::vector<int>& tiposDaEscola) {
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
        bool deuCerto = escolherTiposDasEscolas(
            capacidades, conjuntos, escola + 1, cabe, falhas, tiposDaEscola);
        for (int B = 0; B < todos; ++B) {
            if ((B & S) == S) cabe[B] += capacidades[escola];
        }
        if (deuCerto) {
            tiposDaEscola[escola] = S;
            return true;
        }
    }

    falhas.insert(chave);
    return false;
}

struct ArestaFluxo {
    int destino;
    int capacidade;
    int reversa;
};

static void adicionarArestaFluxo(
    std::vector<std::vector<ArestaFluxo>>& grafo,
    int origem,
    int destino,
    int capacidade
) {
    ArestaFluxo ida = {destino, capacidade, (int)grafo[destino].size()};
    ArestaFluxo volta = {origem, 0, (int)grafo[origem].size()};
    grafo[origem].push_back(ida);
    grafo[destino].push_back(volta);
}

static int calcularFluxo(
    std::vector<std::vector<ArestaFluxo>>& grafo,
    int origem,
    int destino
) {
    int fluxoTotal = 0;
    while (true) {
        std::vector<int> pai(grafo.size(), -1);
        std::vector<int> arestaPai(grafo.size(), -1);
        std::queue<int> fila;
        fila.push(origem);
        pai[origem] = origem;

        while (!fila.empty() && pai[destino] == -1) {
            int atual = fila.front();
            fila.pop();
            for (int i = 0; i < (int)grafo[atual].size(); ++i) {
                const ArestaFluxo& aresta = grafo[atual][i];
                if (aresta.capacidade > 0 && pai[aresta.destino] == -1) {
                    pai[aresta.destino] = atual;
                    arestaPai[aresta.destino] = i;
                    fila.push(aresta.destino);
                }
            }
        }

        if (pai[destino] == -1) break;
        int atual = destino;
        while (atual != origem) {
            int anterior = pai[atual];
            ArestaFluxo& ida = grafo[anterior][arestaPai[atual]];
            --ida.capacidade;
            ++grafo[atual][ida.reversa].capacidade;
            atual = anterior;
        }
        ++fluxoTotal;
    }
    return fluxoTotal;
}

static bool construirSolucaoPorBacktracking(
    std::vector<Candidato>& candidatos,
    std::vector<Escola>& escolas
) {
    std::map<std::string, int> indiceTipo;
    std::vector<std::vector<int>> candidatosPorTipo;
    for (int indice = 0; indice < (int)candidatos.size(); ++indice) {
        const std::string& tipo = candidatos[indice].tipo_prova;
        if (indiceTipo.count(tipo) == 0) {
            indiceTipo[tipo] = candidatosPorTipo.size();
            candidatosPorTipo.push_back(std::vector<int>());
        }
        candidatosPorTipo[indiceTipo[tipo]].push_back(indice);
        candidatos[indice].id_escola_atribuida = -1;
    }

    std::vector<int> capacidades;
    for (const Escola& escola : escolas) {
        capacidades.push_back(escola.capacidadeMaxima);
    }
    const int maxTiposConstrucao = std::min(
        MAX_TIPOS_POR_ESCOLA, (int)candidatosPorTipo.size());
    std::vector<int> conjuntos;
    for (int S = 0; S < (1 << candidatosPorTipo.size()); ++S) {
        int quantidade = 0;
        for (int tipo = 0; tipo < (int)candidatosPorTipo.size(); ++tipo) {
            if (S & (1 << tipo)) ++quantidade;
        }
        if (quantidade == maxTiposConstrucao) conjuntos.push_back(S);
    }
    std::vector<int> demandas;
    for (const std::vector<int>& grupo : candidatosPorTipo) demandas.push_back(grupo.size());
    int capacidadeTotal = 0;
    int demandaTotal = 0;
    for (int capacidade : capacidades) capacidadeTotal += capacidade;
    for (int demanda : demandas) demandaTotal += demanda;
    int folga = capacidadeTotal - demandaTotal;
    std::vector<int> cabe(1 << candidatosPorTipo.size(), folga);
    for (int B = 0; B < (int)cabe.size(); ++B) {
        for (int tipo = 0; tipo < (int)demandas.size(); ++tipo) {
            if (B & (1 << tipo)) cabe[B] += demandas[tipo];
        }
    }
    std::vector<int> tiposDaEscola(escolas.size(), 0);
    std::set<std::vector<int>> falhas;
    if (!escolherTiposDasEscolas(
            capacidades, conjuntos, 0, cabe, falhas, tiposDaEscola)) {
        return false;
    }

    const int numTipos = candidatosPorTipo.size();
    const int numEscolas = escolas.size();
    const int primeiroTipo = 1;
    const int primeiraEscola = primeiroTipo + numTipos;
    const int destino = primeiraEscola + numEscolas;
    std::vector<std::vector<ArestaFluxo>> grafo(destino + 1);
    for (int tipo = 0; tipo < numTipos; ++tipo) {
        adicionarArestaFluxo(
            grafo, 0, primeiroTipo + tipo, candidatosPorTipo[tipo].size());
        for (int escola = 0; escola < numEscolas; ++escola) {
            if (tiposDaEscola[escola] & (1 << tipo)) {
                adicionarArestaFluxo(
                    grafo, primeiroTipo + tipo, primeiraEscola + escola,
                    candidatosPorTipo[tipo].size());
            }
        }
    }
    for (int escola = 0; escola < numEscolas; ++escola) {
        adicionarArestaFluxo(
            grafo, primeiraEscola + escola, destino,
            escolas[escola].capacidadeMaxima);
    }

    if (calcularFluxo(grafo, 0, destino) != demandaTotal) return false;
    for (Escola& escola : escolas) escola.candidatos_alocados.clear();
    for (int tipo = 0; tipo < numTipos; ++tipo) {
        int noTipo = primeiroTipo + tipo;
        for (const ArestaFluxo& aresta : grafo[noTipo]) {
            if (aresta.destino < primeiraEscola || aresta.destino >= destino) continue;
            int escola = aresta.destino - primeiraEscola;
            int quantidade = grafo[aresta.destino][aresta.reversa].capacidade;
            while (quantidade-- > 0) {
                int indice = candidatosPorTipo[tipo].back();
                candidatosPorTipo[tipo].pop_back();
                escolas[escola].candidatos_alocados.push_back(indice);
                candidatos[indice].id_escola_atribuida = escolas[escola].id;
            }
        }
    }
    return true;
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
    std::vector<int> tiposDaEscola(capacidades.size(), 0);
    return escolherTiposDasEscolas(
        capacidades, conjuntos, 0, cabe, falhas, tiposDaEscola);
}
// apenas para validar instâncias, feito por IA

int criarSolucaoInicial(std::vector<Candidato>& candidatos, std::vector<Escola>& escolas) {
    if (!instanciaViavel(candidatos, escolas, MAX_TIPOS_POR_ESCOLA)) {
        return -1;
    }

    return construirSolucaoPorBacktracking(candidatos, escolas) &&
                   validarSolucao(candidatos, escolas)
               ? 0
               : -1;
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
