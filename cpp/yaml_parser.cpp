#include "yaml-cpp/yaml.h"
#include <stdio.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <string.h>


#include "automata.h"

using namespace std;

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


void yamlStringFormater( char *msg, string recog, string rest ) {
  sprintf(msg, MSG_FORMAT, recog.c_str(), rest.c_str() );
}
void yamlCodeStringFormater ( int codterm, char *msg, string recog, string rest ) {
  sprintf(msg, CODE_MSG_FORMAT, codterm, recog.c_str(), rest.c_str());
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

void sendMsg(vector <automata_desc> &lista_automatas, string cmd, string msg){  
    char *_msg = (char*)malloc(sizeof(char)*MAX_WORD_LENGTH);
    memset(_msg, '\0', MAX_WORD_LENGTH);

  if(cmd.compare(diccionario[INFO]) == 0){
    for(unsigned i = 0; i < lista_automatas.size(); i++){       
      if(msg.length()== 0){
        nodes_printer(lista_automatas[i], _msg);
      }else if(msg.compare(lista_automatas[i].nombre)== 0){
        nodes_printer(lista_automatas[i], _msg);
        break;
      }
    }
  }else if(cmd.compare(diccionario[SEND]) == 0){
    yamlStringFormater(_msg, "", msg);
    for(unsigned i = 0; i < lista_automatas.size(); i++){
      vector<nodo_automata> aux= lista_automatas[i].vector_nodos;
      for(unsigned j = 0; j < aux.size(); j++){
        if(aux[j].id.compare(lista_automatas[i].estadoinicial) == 0){
          //es inicial
          //fd[0] lectura - fd[1] escritura  
          write(aux[j].fd[1], _msg, strlen(_msg));
          break;
        }
      }
    }
  }else if(cmd.compare(diccionario[STOP]) == 0){
    kill(0,SIGKILL);
  }
}

int checkCmd (string str){
  if(str.compare(diccionario[INFO])==0){
    return 1;
  }else if(str.compare(diccionario[SEND])==0){
    return 1;
  }else if(str.compare(diccionario[STOP])==0){
    return 1;
  }else{    
    return 0;
  }
}


void lectorComandos(vector <automata_desc> &lista_automatas){  
  string aux;
  //leemos comando.
  while(true){    
    try{
      getline(cin,aux);    
      stringstream str(aux);     
      YAML:: Parser parser(str);
      YAML:: Node node; 
      if (!parser.GetNextDocument (node))
        printErrorMsg("lectorComandos", "Error creando parser");
      string cmd,msg;
      node[diccionario[CMD]] >> cmd;
      if(checkCmd(cmd) != 0){
        node[diccionario[MSG]] >> msg;      
        sendMsg(lista_automatas, cmd, msg);
      }else{
        printErrorMsg("lectorComandos", "Comando no encontrado");
        continue;      
      }
    }catch(YAML::Exception& e){
      printErrorMsg("lectorComandos", e.what());
      continue;
    }           
  }
}

void procesoControlador(nodo_automata *nodo, vector<nodo_automata> &hermanos, vector<string>finales){
  char *buffer = (char *) malloc( sizeof(char) * MAX_WORD_LENGTH);
  memset(buffer, '\0', MAX_WORD_LENGTH);

  char *_msg = (char *) malloc( sizeof(char) * MAX_WORD_LENGTH);
  memset(_msg, '\0', MAX_WORD_LENGTH);
  int send = 0, check = 0;
  //cout << "nombre del automata: " << nodo->automata->nombre << endl;
  while(true){    
    while(read(nodo->fd[0], buffer,MAX_WORD_LENGTH)>0){

      _msg[0]='\0';
      send = 0;
      check = 0;
      try{
        stringstream str(buffer);
        YAML:: Parser parser(str);
        YAML:: Node node;      
        if (!parser.GetNextDocument (node))
          printErrorMsg("procesoControlador", "Error creando parser");
        string recog, rest;
        node[diccionario[RECOG]] >> recog;      
        node[diccionario[REST]] >> rest;      
        if(!rest.empty()){
          vector<transicion_nodos> aux = nodo->list_transiciones;
          for(unsigned i=0; i<aux.size(); i++){
            if(rest.compare(0,aux[i].entrada.length(),aux[i].entrada)==0){
              recog.append(aux[i].entrada);     
              rest.erase(0,aux[i].entrada.length());                          
              yamlStringFormater(_msg, recog, rest);            
              for(unsigned j =0; j<hermanos.size(); j++){
                if((hermanos[j].id.compare(aux[i].sig_estado))==0){                
                  write(hermanos[j].fd[1], _msg, strlen(_msg));
                  send=1;
                  break;
                }
              }
              break;
            }
          }
        }
        if(!send){
          for(unsigned k=0; k<finales.size();k++){
            if(nodo->id.compare(finales[k])==0 && rest.length() ==0){
              yamlCodeStringFormater(0, _msg, recog, rest);
              check = 1;
              break;
            }
          }
          if(!check){
            yamlCodeStringFormater(1, _msg, recog, rest);
          }
          write(nodo->pipe_to_father[1], _msg, strlen(_msg));
        }
        }catch(YAML::Exception& e){
          printErrorMsg("Main", e.what());          
        }
    }
  }
}

int crearProcesos(vector<nodo_automata> &nodos, vector<string> finales, int nodo){  

  for (unsigned i = 0; i < nodos.size(); ++i){
    nodo_automata *temp = &nodos[i];
    if(pipe(fd_padre[nodo])==-1){
      //hubo error.      
      printErrorMsg("Crear Procesos", "No se pudieron crear los PIPES");
    }else{
      temp->fd = fd_padre[nodo];
      nodo++;
    }
  }
  pid_t p_id;    
  for (unsigned i = 0; i < nodos.size(); ++i)
  {
    nodo_automata *temp = &nodos[i];
    if( (p_id = fork() ) == 0){
      //dentro del hijo
      procesoControlador(temp, nodos, finales);
      break;
    }else{
      //le asignamos al nodo este id. cada nodo conoce su id asignado
      temp->pid=p_id;
    }
  }  
  return nodo;
}

void* readingThreadController(void *args){
  automata_desc *pautomata = (automata_desc*)args;
  char *buffer = (char *) malloc( sizeof(char) * MAX_WORD_LENGTH);
  memset(buffer, '\0', MAX_WORD_LENGTH);
  
  //cout << "nombre del automata: " << nodo->automata->nombre << endl;
  while(true){
    while(read(pautomata->pipe_to_father[0], buffer,MAX_WORD_LENGTH)>0){      
      stringstream str(buffer);
      YAML:: Parser parser(str);
      YAML:: Node node;      
      if (!parser.GetNextDocument (node))
        printErrorMsg("readingThreadController", "Error creando parser");
      string recog, rest;
      int codterm;
      node[diccionario[CODTERM]] >> codterm;      
      node[diccionario[RECOG]] >> recog;    
      if(codterm == 0){
        printAcceptMsg(pautomata->nombre, recog);
        break;
      }  
      node[diccionario[REST]] >> rest;
      if(codterm == 1){
        printRejectMsg(pautomata->nombre, recog+rest, recog.length()+1);
        break;
      }
    }
  }    
}

void crearHijos (vector <automata_desc> &lista_automatas){
  int nodo = 0;
  fd_padre = (int**) malloc(MAX_AUTOMATAS*sizeof(int*));
  for(unsigned i = 0; i < MAX_AUTOMATAS; i++){    
    fd_padre[i] = (int*) malloc(sizeof(int)*2);
  }
  for(unsigned i = 0; i < lista_automatas.size(); i++){    
    automata_desc *automata;
    automata = &lista_automatas[i];
    pthread_create(&automata->hilo_lectura,NULL, readingThreadController,(void *)automata);
    nodo = crearProcesos(automata->vector_nodos, automata->final, nodo);    
  }
  lectorComandos(lista_automatas);
}

int main(int argc, char const *argv[]){
//creamos el grupo de procesos.
  setpgid(getpid(), getpid());  
  if ( argc < 2 ){    
    printErrorMsg("Main", "Error, no hay archivo.");
    return SYSTEM_ERROR;
  }
  // Parseando el Yaml
  try{
    ifstream yaml_file(argv[1]);
    YAML:: Parser parser(yaml_file);
    YAML:: Node node;
    parser.GetNextDocument(node);

    vector <automata_desc> lista_automatas;
    for(unsigned i = 0; i < node.size(); i++){
      automata_desc automata;
      automata.pipe_to_father= (int*)malloc(sizeof(int)*2);
      if(pipe(automata.pipe_to_father)== -1){
        //Hubo un error.
        printErrorMsg("Main", "Error creando el PIPE al padre");
      }
      node[i] >> automata;
      //en cada posición del arreglo va a tener un arreglo de dos posiciones.
      lista_automatas.push_back(automata);    
    } 
    crearHijos(lista_automatas); 
  }catch(YAML::Exception& e){
    printErrorMsg("Main", e.what());
  } 
  return SYSTEM_SUCCESS;
}