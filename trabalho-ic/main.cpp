#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <limits>
#include <algorithm>

int K_VIZINHOS = 1;
int PESO_TIPO_PROVA = 2;

struct Candidato {
    std::string id;
    std::string cep;
    std::string tipo_prova;
    double latitude;
    double longitude;
    int id_escola_atribuida;
};

struct Escola {
    int id;
    double latitude;
    double longitude;
    std::vector<int> candidatos_alocados;
    int capacidadeMaxima;
    bool estaCheia;
    int quantidadeTipoProvas;
};

struct Grupo { 
    int id; 
    double latitude; 
    double longitude; 
    std::string tipo_prova; 
    std::vector<int> candidatos_alocados; 
}; 

double calcularDistancia(double lat1, double lon1, double lat2, double lon2, std::string tipo_prova1, std::string tipo_prova2) {
    double distanciaEuclidiana = std::sqrt(std::pow(lat1 - lat2, 2) + std::pow(lon1 - lon2, 2));
    int distanciaTipoProva = (tipo_prova1 != tipo_prova2) ? PESO_TIPO_PROVA : 0;
    double distanciaTotal = std::sqrt(std::pow(distanciaEuclidiana, 2) + std::pow(distanciaTipoProva, 2));
    return distanciaTotal;
}

void processarLinhaCandidato(const std::string& linha, std::vector<Candidato>& candidatos) {
    std::stringstream ss(linha);
    std::string item;
    Candidato c;
    c.id_escola_atribuida = -1;

    std::getline(ss, c.id, ',');
    
    std::getline(ss, item, ',');
    c.cep = item;
    
    std::getline(ss, item, ',');
    c.tipo_prova = item;
    
    std::getline(ss, item, ',');
    c.latitude = std::stod(item);
    
    std::getline(ss, item, ',');
    c.longitude = std::stod(item);

    candidatos.push_back(c);
}

void processarLinhaEscola(const std::string& linha, std::vector<Escola>& escolas) { 
    std::stringstream ss(linha); 
    std::string item; 
    Escola e; 
    
    std::getline(ss, item, ','); e.id = std::stoi(item);
    std::getline(ss, item, ','); e.latitude = std::stod(item);
    std::getline(ss, item, ','); e.longitude = std::stod(item); 
    
    escolas.push_back(e); 
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

int main() {
    std::ifstream arquivo("instancia.txt");
    if (!arquivo.is_open()) {
        std::cerr << "Erro" << std::endl;
        return 1;
    }

    int num_candidatos = 0;
    int num_escolas = 0;
    std::string linha;

    if (std::getline(arquivo, linha)) {
        size_t pos_candidatos = linha.find("NUM_CANDIDATOS:");
        size_t pos_escolas = linha.find("NUM_ESCOLAS:");
        
        if (pos_candidatos != std::string::npos && pos_escolas != std::string::npos) {
            std::string str_cand = linha.substr(pos_candidatos + 15, pos_escolas - (pos_candidatos + 15));
            std::string str_esc = linha.substr(pos_escolas + 12);
            num_candidatos = std::stoi(str_cand);
            num_escolas = std::stoi(str_esc);
        }
    }

    std::vector<Candidato> candidatos;
    std::vector<Escola> escolas; 
    int lidos_candidatos = 0; 

    while (std::getline(arquivo, linha)) {
        if (linha.empty() || linha[0] == '#') continue;
        
        if (lidos_candidatos < num_candidatos) { 
            processarLinhaCandidato(linha, candidatos);
            lidos_candidatos++; 
        } else { 
            processarLinhaEscola(linha, escolas); 
        } 
    }
    arquivo.close();

    std::vector<Grupo> grupos; 
    for (int i = 0; i < num_escolas && i < candidatos.size(); ++i) { 
        Grupo g; 
        g.id = i;
        g.latitude = candidatos[i].latitude; 
        g.longitude = candidatos[i].longitude;
        g.tipo_prova = candidatos[i].tipo_prova;
        grupos.push_back(g); 
    } 

    agruparCandidatosKNN(candidatos, grupos); 

    alocarGruposEscolas(grupos, escolas, candidatos); 

    return 0;
}