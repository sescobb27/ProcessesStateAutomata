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
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#define YAML_ERROR 0
#define YAML_SUCCESS 1
#define SYSTEM_SUCCESS 0
#define SYSTEM_ERROR 1
#define FILE_ERROR 0
#define ALPHA_SIZE 20
#define FIN_YAML 0
#define NO_FIN_YAML 1
#define MAX_WORD_LENGTH 500
#define MAX_AUTOMATAS 20

  // FORMATS
#define MSG_FORMAT "{ recog: %s, rest: %s }"
#define CODE_MSG_FORMAT "{ codterm: %d, recog: %s, rest: %s }"
#define INFO_FORMAT       "- msgtype: info\n  info:\n    - automata: %s\n      ppid: %d\n"
#define ACCEPT_FORMAT   "- msgtype: accept\n  accept:\n     - automata: %s\n       msg: %s\n"
#define REJECT_FORMAT    "- msgtype: reject\n  reject:\n     - automata: %s\n       msg: %s\n       pos: %d\n"
#define ERROR_FORMAT    "- msgtype: error\n  error:\n    - where: \"%s\"\n      cause: \"%s\"\n"
#define NODE_MSG_FORMAT "      - node: %s\n        pid: %d\n"

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
    pid_t pid;
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
    int *pipe_to_father;    
};

//matriz de enteros.
int **fd_padre;

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


void yamlStringFormater( char *msg, char *recog, char *rest ) {
  sprintf(msg, MSG_FORMAT, recog, rest );
}
void yamlCodeStringFormater ( int codterm, char *msg, char *recog, char *rest ) {
  sprintf(msg, CODE_MSG_FORMAT, codterm, recog, rest );
}

void yamlInfoNode( char *info, const char *id, int ppid ) {
  sprintf( info, NODE_MSG_FORMAT, id, ppid );
}

void printInfoMsg(string automata_name) {
  fprintf(stdout, INFO_FORMAT, automata_name.c_str(), getpid());
}

void printAcceptMsg( string automata_name, string msg ) {
  fprintf(stdout, ACCEPT_FORMAT, automata_name.c_str(), msg.c_str() );
}

void printRejectMsg( string automata_name, string msg, int pos ) {
  fprintf(stdout, REJECT_FORMAT, automata_name.c_str(), msg.c_str(), pos );
}

void printErrorMsg( string where, string cause ) {
  fprintf(stderr, ERROR_FORMAT, where.c_str(), cause.c_str() );
}

void nodes_printer(automata_desc automata, char *info_msg) {
  printInfoMsg(automata.nombre);
  vector <nodo_automata> lista_aux = automata.vector_nodos;
  for (unsigned i = 0; i < lista_aux.size(); i++) {
    yamlInfoNode( info_msg, lista_aux[i].id.c_str(), lista_aux[i].pid);
    fprintf(stdout, "%s", info_msg);
  }
}

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
    nodoAutomata.pipe_to_father = automata.pipe_to_father;
    automata.vector_nodos.push_back(nodoAutomata);
  }
}

void sendMsg(vector<automata_desc>lista_automatas, string cmd, string msg){  
  if(strcmp(cmd.c_str(), diccionario[INFO].c_str()) == 0){
    char *_msg = (char*)malloc(sizeof(char)*MAX_WORD_LENGTH);
    memset(_msg, '\0', MAX_WORD_LENGTH);   
    for(unsigned i = 0; i < lista_automatas.size(); i++){       
      if(msg.length()== 0){
        nodes_printer(lista_automatas[i], _msg);
      }else if(strcmp(_msg, lista_automatas[i].nombre.c_str())){
        nodes_printer(lista_automatas[i], _msg);
        break;
      }
    }
  }else if(strcmp(cmd.c_str(), diccionario[SEND].c_str()) == 0){
    
  }else if(strcmp(cmd.c_str(), diccionario[STOP].c_str()) == 0){
    kill(0,SIGKILL);
  }
}

int checkCmd (string str){
  if(strcmp(str.c_str(), diccionario[INFO].c_str())==0){
    return 1;
  }else if(strcmp(str.c_str(), diccionario[SEND].c_str())==0){
    return 1;
  }else if(strcmp(str.c_str(), diccionario[STOP].c_str())==0){
    return 1;
  }else{    
    return 0;
  }
}


void lectorComandos(vector<automata_desc>lista_automatas){  
  string aux;
  //leemos comando.
  while(true){    
    getline(cin,aux);    
    stringstream str(aux);
   
    YAML:: Parser parser(str);
    YAML:: Node node;      
    if (!parser.GetNextDocument (node))
      cerr << "cant parse: \n";
    string cmd,msg;
    node[diccionario[CMD]] >> cmd;
    if(checkCmd(cmd) != 0){
      node[diccionario[MSG]] >> msg;      
      sendMsg(lista_automatas, cmd, msg);
    }else{
      cout << "command not found" << endl;
      continue;      
    }
    //cout << cmd << ": " << msg << endl;     
  }
}

void procesoControlador(nodo_automata *nodo, string nombre_automata, vector<string>finales){
  cout << nombre_automata << ": " << (*nodo).id << ", " << getpid() << endl;  
  while(true){

  }
}

int crearProcesos(vector <nodo_automata> nodos, string nombre_automata, vector <string> finales, int nodo){  

  for (unsigned i = 0; i < nodos.size(); ++i){

    if(pipe(fd_padre[nodo])==-1){
      //hubo error.
      cout << "Error creando pipes en crear Procesos " << endl;
    }else{
      nodos[i].fd = fd_padre[nodo];
      nodo++;
    }
  }
  pid_t p_id;
  for (unsigned i = 0; i < nodos.size(); ++i)
  {
    if( ( nodos[i].pid = fork() ) == 0){
      //dentro del hijo
      procesoControlador(&nodos[i], nombre_automata, finales);
      break;
    }else{
      //le asignamos al nodo este id. cada nodo conoce su id asignado
      //nodos[i].pid = p_id;
    }
  }  
  return nodo;
}

void crearHijos (vector <automata_desc> lista_automatas){
  int nodo = 0;
  fd_padre = (int**) malloc(MAX_AUTOMATAS*sizeof(int*));
  for(unsigned i = 0; i < MAX_AUTOMATAS; i++){
    fd_padre[i] = (int*) malloc(sizeof(int)*2);
  }
  for(unsigned i = 0; i < lista_automatas.size(); i++){
    automata_desc automata;
    automata = lista_automatas[i];
    nodo = crearProcesos(automata.vector_nodos, automata.nombre, automata.final, nodo);    
  }
  lectorComandos(lista_automatas);
}

int main(int argc, char const *argv[]){
//creamos el grupo de procesos.
  setpgid(getpid(), getpid());  
  if ( argc < 2 ){
    fprintf(stdout, "There is no file to parse, try again\n");
    return SYSTEM_ERROR;
  }
  // Parseando el Yaml
  ifstream yaml_file(argv[1]);
  YAML:: Parser parser(yaml_file);
  YAML:: Node node;
  parser.GetNextDocument(node);
  vector <automata_desc> automatas;
  for(unsigned i = 0; i < node.size(); i++){
    automata_desc automata;
    automata.pipe_to_father= (int*)malloc(sizeof(int)*2);
    if(pipe(automata.pipe_to_father)== -1){
      //Hubo un error.
      cout << "Error creando Pipe" << endl;
    }
    node[i] >> automata;
    //en cada posición del arreglo va a tener un arreglo de dos posiciones.
    automatas.push_back(automata);    
    /*
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
    */
  } 
  crearHijos(automatas);  
  return SYSTEM_SUCCESS;
}