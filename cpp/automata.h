

#include <pthread.h>
#include <unistd.h>
#include <string>
#include <vector>
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
#define MSG_FORMAT "{ recog: \"%s\", rest: \"%s\" }"
#define CODE_MSG_FORMAT "{ codterm: %d, recog: \"%s\", rest: \"%s\" }"
#define INFO_FORMAT       "- msgtype: info\n  info:\n    - automata: %s\n      ppid: %d\n      nodes:\n"
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
    pthread_t hilo_lectura;   
};



void yamlstringFormater( char *msg, string recog, string rest );
void yamlCodestringFormater ( int codterm, char *msg, string recog, string rest );
void yamlInfoNode( char *info, const char *id, int ppid );
void printInfoMsg(string automata_name);
void printAcceptMsg( string automata_name, string msg );
void printRejectMsg( string automata_name, string msg, int pos );
void printErrorMsg( string where, string cause );
void printErrorMsg( string where, string cause );

void sendMsg(vector <automata_desc> &lista_automatas, string cmd, string msg);
int checkCmd (string str);
void lectorComandos(vector <automata_desc> &lista_automatas);
void procesoControlador(nodo_automata *nodo, vector<nodo_automata> &hermanos, vector<string>finales);
int crearProcesos(vector<nodo_automata> &nodos, vector<string> finales, int nodo);
void* readingThreadController(void *args);
void crearHijos (vector <automata_desc> &lista_automatas);