#include "yaml-cpp/yaml.h"
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <sys/wait.h>
#include <signal.h>
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
using namespace std;

/*
* TAGS para determinar el diccionario
*/
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
  NEXT,
  // user command parser
  MSG,
  CMD,
  INFO,
  SEND,
  STOP,
  REST,
  RECOG,
  CODTERM
};

//ESTRUCTURAS


/*
*Estructura para el flujo de nodos
*/
struct transicion_nodos {
    string entrada;
    string sig_estado;     
};
/*
*Estructura la definición del automata
*/
struct nodo_automata{
    string id;
    int *fd;
    int *pipe_to_father;    
    list<transicion_nodos> list_transiciones;

};
/*
*Estructura para la definición del campo descripción
*en el archivo yaml
*/
struct automata_desc{
    string  nombre;
    string descripcion;
    list<string> alfabeto;
    int sizeAlfabeto;
    list<string> estados;
    int sizeEstados;
    string estadoinicial;
    list<string> final;
    int sizeFinal;
    list<nodo_automata> nodos_automata;
};

typedef enum tags tag;

//diccionario de TAGS en el archivo yaml
string diccionario[20]={
  "automata",
  "descripcion",
  "alpha",
  "states",
  "start",
  "final",
  "delta",
  "node",
  "trans",
  "in",
  "next",
  "msg",
  "cmd",
  "info",
  "send",
  "stop",
  "rest",
  "recog",
  "codterm"
};

typedef struct automata_desc automata;
typedef struct automata_desc* p_type_automata;
typedef struct nodo_automata nodo;
typedef struct nodo_automata* p_type_nodo;
typedef struct flujo_nodos flujo;
typedef struct flujo_nodos* p_type_flujo;
typedef struct transicion_nodos transicion;
typedef struct transicion_nodos* p_type_transicion;

list <p_type_automata> automatas;

//Extracción de la entrada y el siguiente estado de cada nodo
void operator >> (const YAML::Node& node, transicion_nodos& transicion_nodos){
  node[diccionario[IN]] >> transicion_nodos.entrada;
  node[diccionario[NEXT]] >> transicion_nodos.sig_estado;
}

//Extracción de los componentes de un automata
void operator >> (const YAML::Node& node, nodo_automata& nodo_automata ){
  node[diccionario[AUTOMATA]] >> nodo_automata.id;
  const YAML::Node& list_transiciones = node[diccionario[TRANS]];
  for(int i = 0;list_transiciones.size();i++){
    transicion_nodos transicion;
    list_transiciones[i] >> transicion;
    nodo_automata.list_transiciones.push_back(transicion);
  }
}

//Extracción de los componentes en el Delta
void operator >> (const YAML::Node& node, automata_desc& automata_desc){
  node[diccionario[AUTOMATA]] >> automata_desc.nombre;
  node[diccionario[DESCRIPTION]] >> automata_desc.descripcion;
  //node[diccionario[ALPHA]] >> automata_desc.alpha;
}

int main(int argc, char const *argv[]){
  if ( argc < 2 ){
    fprintf(stdout, "There is no file to parse, try again\n");
    return SYSTEM_ERROR;
  }
  // Parseando el Yaml
  std::ifstream yaml_file(argv[1]);
  YAML:: Parser parser(yaml_file);
  YAML:: Node node;
  parser.GetNextDocument(node);

  
  return SYSTEM_SUCCESS;
}