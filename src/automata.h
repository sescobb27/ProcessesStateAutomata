#include "yaml.h"
#include <unistd.h>
#include <pthread.h>
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

struct transicion_nodos {
    char *entrada;
    char *sig_estado;
    struct transicion_nodos *next;
};

struct nodo_automata{
    char *id;
    int *fd;
    int *pipe_to_father;
    pid_t pid;
    struct transicion_nodos *transiciones;
    struct transicion_nodos *primer_transicion;
    struct nodo_automata *next;
    struct nodo_automata *primer_nodo;
};

struct automata_desc{
    char *nombre;
    char *descripcion;
    char **alfabeto;
    int sizeAlfabeto;
    char **estados;
    int sizeEstados;
    char *estadoinicial;
    char **final;
    int sizeFinal;
    int *pipe_to_father;
    pthread_t hilo_lectura;
    struct nodo_automata *nodos;
    struct nodo_automata *primer_nodo;
    struct automata_desc *next;
};

// TAGS para determinar el diccionario
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

typedef struct automata_desc automata;
typedef struct automata_desc* p_type_automata;
typedef struct nodo_automata nodo;
typedef struct nodo_automata* p_type_nodo;
typedef struct transicion_nodos transicion;
typedef struct transicion_nodos* p_type_transicion;
typedef enum tags tag;

// definicion de metodos
void nodes_printer(p_type_automata pautomata, char *info_msg);
int yamlParser(yaml_event_t *event, yaml_parser_t *parser, char *where );
int deleteEvent(yaml_event_t *event);
void next(yaml_event_t *event, yaml_parser_t *parser, char *where);
void initStringArray(p_type_automata *pautomata);
char* getCommand( char* data );
void yamlStringFormater( char *msg, char *recog, char *rest );
void yamlCodeStringFormater ( int codterm, char *msg, char *recog, char *rest );
void yamlInfoNode( char *info, char *id, int ppid );
void printInfoMsg(char *automata_name);
void printAcceptMsg( char *automata_name, char *msg );
void printRejectMsg( char *automata_name, char *msg, int pos );
void printErrorMsg( char *where, const char *cause );
void* readingThreadController(void *args);
void sendCommand(char *command, char *msg, p_type_automata pautomata);
void startListeningUserInput( p_type_automata pautomata);
void processController(p_type_nodo *pnodo, char* nombre_automata, char **estados_finales, int size_finales);
int createProcessPerNode(p_type_nodo pnodo, char* nombre_automata, int nodo, char **estados_finales, int size_finales);
void crearHijos( p_type_automata pautomata);
void parseTransitions(yaml_parser_t *parser, yaml_event_t *event, p_type_nodo *pnodo);
void parseNodesSection(yaml_parser_t *parser, yaml_event_t *event, p_type_automata *pautomata);
void parseSequenceSection(yaml_event_t *event, yaml_parser_t *parser, int kind, p_type_automata *pautomata);
p_type_automata automataParser(yaml_event_t *event, yaml_parser_t *parser);
p_type_automata startParsingYamlFile(yaml_parser_t *parser);