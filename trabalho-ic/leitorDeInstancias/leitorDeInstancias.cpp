#include "leitorDeInstancias.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

static std::string aparar(const std::string& s) {
    size_t ini = s.find_first_not_of(" \t\r");
    if (ini == std::string::npos) return "";
    size_t fim = s.find_last_not_of(" \t\r");
    return s.substr(ini, fim - ini + 1);
}

static std::vector<std::string> separar(const std::string& linha, char separador) {
    std::vector<std::string> campos;
    std::stringstream ss(linha);
    std::string item;
    while (std::getline(ss, item, separador)) {
        campos.push_back(aparar(item));
    }
    return campos;
}

// stoi/stod aceitam lixo no final ("2.25" vira 2); aqui o campo inteiro precisa ser número
static int paraInt(const std::string& s) {
    size_t lidos = 0;
    int valor = std::stoi(s, &lidos);
    if (lidos != s.size()) throw std::invalid_argument("inteiro inválido: '" + s + "'");
    return valor;
}

static double paraDouble(const std::string& s) {
    size_t lidos = 0;
    double valor = std::stod(s, &lidos);
    if (lidos != s.size()) throw std::invalid_argument("número inválido: '" + s + "'");
    return valor;
}

static void exigirCampos(const std::vector<std::string>& campos, size_t esperado, const std::string& oQue) {
    if (campos.size() != esperado) {
        throw std::invalid_argument(oQue + " com " + std::to_string(campos.size()) +
                                    " campos, esperado " + std::to_string(esperado));
    }
}

// id, CEP, tipo_prova, latitude, longitude
static Candidato lerCandidato(const std::string& linha) {
    std::vector<std::string> campos = separar(linha, ',');
    exigirCampos(campos, 5, "candidato");

    Candidato c;
    c.id = campos[0];
    c.cep = campos[1];
    c.tipo_prova = campos[2];
    c.latitude = paraDouble(campos[3]);
    c.longitude = paraDouble(campos[4]);
    return c;
}

// id, CEP, latitude, longitude, num_salas, capacidades_salas (separadas por ';')
static Escola lerEscola(const std::string& linha) {
    std::vector<std::string> campos = separar(linha, ',');
    exigirCampos(campos, 6, "escola");

    Escola e;
    size_t inicio_numero = campos[0].find_first_of("0123456789");
    if (inicio_numero == std::string::npos) throw std::invalid_argument("id de escola sem número: '" + campos[0] + "'");
    e.id = paraInt(campos[0].substr(inicio_numero));
    e.cep = campos[1];
    e.latitude = paraDouble(campos[2]);
    e.longitude = paraDouble(campos[3]);

    int num_salas = paraInt(campos[4]);
    for (const std::string& cap : separar(campos[5], ';')) {
        e.capacidades_salas.push_back(paraInt(cap));
        e.capacidadeMaxima += e.capacidades_salas.back();
    }
    if ((int)e.capacidades_salas.size() != num_salas) {
        throw std::invalid_argument("escola " + campos[0] + " declara " + std::to_string(num_salas) +
                                    " salas mas lista " + std::to_string(e.capacidades_salas.size()));
    }
    return e;
}

static std::vector<double> lerLinhaMatriz(const std::string& linha, int num_escolas) {
    std::vector<std::string> campos = separar(linha, ',');
    exigirCampos(campos, num_escolas, "linha da matriz");

    std::vector<double> distancias;
    for (const std::string& d : campos) distancias.push_back(paraDouble(d));
    return distancias;
}

// "NUM_CANDIDATOS: 4561 NUM_ESCOLAS: 15"
static void lerCabecalho(const std::string& linha, Instancia& instancia) {
    std::stringstream ss(linha);
    std::string rotulo;
    bool achou_candidatos = false, achou_escolas = false;
    while (ss >> rotulo) {
        if (rotulo == "NUM_CANDIDATOS:") achou_candidatos = static_cast<bool>(ss >> instancia.num_candidatos);
        else if (rotulo == "NUM_ESCOLAS:") achou_escolas = static_cast<bool>(ss >> instancia.num_escolas);
    }
    if (!achou_candidatos || !achou_escolas) {
        throw std::invalid_argument("cabeçalho inválido, esperado 'NUM_CANDIDATOS: N NUM_ESCOLAS: M'");
    }
}

bool lerInstancia(const std::string& caminho, Instancia& instancia) {
    std::ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        std::cerr << "Erro: não foi possível abrir '" << caminho << "'" << std::endl;
        return false;
    }

    enum Secao { CABECALHO, CANDIDATOS, ESCOLAS, MATRIZ };
    Secao secao = CABECALHO;
    std::string linha;
    int num_linha = 0;

    try {
        while (std::getline(arquivo, linha)) {
            ++num_linha;
            linha = aparar(linha);
            if (linha.empty()) continue;

            if (linha[0] == '#') {
                if (linha.find("MATRIZ") != std::string::npos) secao = MATRIZ;
                else if (linha.find("CANDIDATOS") != std::string::npos) secao = CANDIDATOS;
                else if (linha.find("ESCOLAS") != std::string::npos) secao = ESCOLAS;
                continue;
            }

            switch (secao) {
                case CABECALHO:
                    lerCabecalho(linha, instancia);
                    break;
                case CANDIDATOS:
                    instancia.candidatos.push_back(lerCandidato(linha));
                    break;
                case ESCOLAS:
                    instancia.escolas.push_back(lerEscola(linha));
                    break;
                case MATRIZ:
                    instancia.matriz_distancias.push_back(lerLinhaMatriz(linha, instancia.num_escolas));
                    break;
            }
        }
    } catch (const std::exception& e) {
        // std::stoi/stod lançam com mensagem curta ("stod"); o número da linha diz onde olhar
        std::cerr << "Erro em " << caminho << ":" << num_linha << ": " << e.what() << std::endl;
        return false;
    }

    if ((int)instancia.candidatos.size() != instancia.num_candidatos ||
        (int)instancia.escolas.size() != instancia.num_escolas ||
        instancia.matriz_distancias.size() != instancia.candidatos.size()) {
        std::cerr << "Erro em " << caminho << ": cabeçalho diz " << instancia.num_candidatos << " candidatos e "
                  << instancia.num_escolas << " escolas; lidos " << instancia.candidatos.size() << " candidatos, "
                  << instancia.escolas.size() << " escolas e " << instancia.matriz_distancias.size()
                  << " linhas de matriz" << std::endl;
        return false;
    }

    return true;
}
