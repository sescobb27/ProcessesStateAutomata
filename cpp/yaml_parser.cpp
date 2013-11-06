#include "yaml-cpp/yaml.h"
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <vector>

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
    vector<transicion_nodos> list_transiciones;

};
/*
*Estructura para la definición del campo descripción
*en el archivo yaml
*/
struct automata_desc{
    string  nombre;
    string descripcion;
    vector<string> alfabeto;    
    vector<string> estados;    
    string estadoinicial;
    vector<string> final;
    vector<nodo_automata> vector_nodos;
};

//typedef enum tags tag;

//diccionario de TAGS en el archivo yaml
string diccionario[20]={
  "automata",
  "description",
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


vector <automata_desc> automatas;

//Extracción de la entrada y el siguiente estado de cada nodo
void operator >> (const YAML::Node& node, transicion_nodos& transicion_nodos){
  node[diccionario[IN]] >> transicion_nodos.entrada;
  node[diccionario[NEXT]] >> transicion_nodos.sig_estado;
}

//Extracción de los componentes de un automata
void operator >> (const YAML::Node& node, nodo_automata& nodoAutomata){
  node[diccionario[NODE]] >> nodoAutomata.id;
  const YAML::Node& transiciones = node[diccionario[TRANS]];
  for(unsigned i = 0;i < transiciones.size();i++){
    transicion_nodos transicion;
    transiciones[i] >> transicion;
    nodoAutomata.list_transiciones.push_back(transicion);
  }
}

void operator >> (const YAML::Node& node, automata_desc& automata) {
  node[diccionario[AUTOMATA]] >> automata.nombre;
  node[diccionario[DESCRIPTION]] >> automata.descripcion;
  node[diccionario[ALPHA]] >> automata.alfabeto;
  node[diccionario[STATES]] >> automata.estados;
  node[diccionario[START]] >> automata.estadoinicial;
  node[diccionario[FINAL]] >> automata.final;
  const YAML::Node& list_nodos = node[diccionario[DELTA]];     
  for(unsigned j = 0; j < list_nodos.size(); j++){
    nodo_automata nodoAutomata;
    list_nodos[j] >> nodoAutomata;
    automata.vector_nodos.push_back(nodoAutomata);
  }
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
  for(unsigned i = 0; i < node.size(); i++){
    automata_desc automata;
    node[i] >> automata;
    automatas.push_back(automata);
    cout << "Nombre Automata: " << automata.nombre << endl;
    cout << "descripcion Automata: "<< automata.descripcion << endl;
    for(unsigned k = 0; k < automata.alfabeto.size(); k++){
      cout << "Alfabeto Automata: "<< automata.alfabeto[k] << endl;
    }
    for(unsigned k = 0; k < automata.estados.size(); k++){
      cout << "Estado Automata: "<< automata.estados[k] << endl;
    }
    cout << "Start Automata: "<< automata.estadoinicial << endl;
    for(unsigned k = 0; k < automata.final.size(); k++){
      cout << "Final Automata: "<< automata.final[k] << endl;
    }     
    for(unsigned k = 0; k < automata.vector_nodos.size(); k++){
      cout << "Nodo Automata: "<< automata.vector_nodos[k].id << endl;
    }
  } 
  return SYSTEM_SUCCESS;
}