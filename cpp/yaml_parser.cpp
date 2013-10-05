#include "yaml-cpp/yaml.h"
#include <iostream>
#include <fstream>
#include <string>
#include <list>

#define YAML_ERROR 0
#define YAML_SUCCESS 1
#define SYSTEM_SUCCESS 0
#define SYSTEM_ERROR 1
#define FILE_ERROR 0
#define ALPHA_NUMBER 200
#define STATES_NUMBER 200
#define NUM_CHARS 10
#define FIN_YAML 0
#define NO_FIN_YAML 1

enum tags {
  AUTOMATA,
  DESCRIPTION,
  ALPHA,
  STATES,
  START,
  FINAL,
  DELTA,
  NODE,
  TRANS,
  IN,
  NEXT
};

struct flujo_nodos{
    std::string entrada;
    std::string sig_estado;
};

struct transicion_nodos { 
    std::list<flujo_nodos> list_flujo_nodos;
};

struct nodo_automata{
    char id;
    std::list<transicion_nodos> list_transiciones;
};

struct automata_desc{
    std::string  nombre;
    std::string descripcion;
    std::list<std::string> alfabeto;
    int sizeAlfabeto;
    std::list<std::string> estados;
    int sizeEstados;
    std::string estadoinicial;
    std::list<std::string> final;
    int sizeFinal;
    std::list<nodo_automata> nodos_automata;
};

typedef enum tags tag;

typedef struct automata_desc automata;
typedef struct automata_desc* p_type_automata;
typedef struct nodo_automata nodo;
typedef struct nodo_automata* p_type_nodo;
typedef struct flujo_nodos flujo;
typedef struct flujo_nodos* p_type_flujo;
typedef struct transicion_nodos transicion;
typedef struct transicion_nodos* p_type_transicion;

void parseDescriptionSection( YAML::Node file ){
    p_type_automata pautomata = ( p_type_automata ) malloc( sizeof( automata ) );
    // std::cout << file;
    std::cout << file["automata"].as<std::string>();
    printf("Entro 1\n");
    const std::string name= file["automata"].as<std::string>();
    printf("Entro 2\n");
    pautomata->descripcion = file["description"].as<std::string>();
    printf("Entro 3\n");
    pautomata->alfabeto = file["alpha"].as< std::list<std::string> >();
    printf("Entro 4\n");
    pautomata->estados = file["states"].as< std::list<std::string> >();
    printf("Entro 5\n");
    pautomata->estadoinicial = file["start"].as<std::string>();
    printf("Entro 6\n");
    pautomata->final = file["final"].as<std::list<std::string> >();
    std::cout << "Nombre: \n" << pautomata->nombre;
    std::cout << "Descripcion: \n" << pautomata->descripcion;
    std::cout << "Alfabeto: \n";
    std::list<std::string>::const_iterator it = pautomata->alfabeto.begin();
    for (; it != pautomata->alfabeto.end(); ++it)
    {
      std::cout << it->c_str();
    }
}

int main(int argc, char const *argv[]){
  if ( argc < 2 ){
    fprintf(stdout, "There is no file to parse, try again\n");
    return SYSTEM_ERROR;
  }
  FILE *yaml_file = fopen( argv[1], "r" );
  YAML::Node file = YAML::Load(argv[1]);
  // YAML::Node file = YAML::LoadFile(argv[1]);
  parseDescriptionSection( file );
  std::ofstream fout(argv[1]);
  fout << file;
  return SYSTEM_SUCCESS;
}