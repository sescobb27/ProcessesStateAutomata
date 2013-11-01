/*   USAR cc -o name yaml_parser.c -lyaml   LUEGO ./name file.yaml

  cc yaml_parser.c -lyaml
  gcc yaml_parser.c -lyaml
  gcc -o name yaml_parser.c -lyaml
  ./name file.yaml

  poner dentro del make CFLAGS="-I/usr/include/glib-2.0'
  cc `pkg-config --libs --cflags glib-2.0` -o parser yaml_parser.c -lyaml
  pkg-config --list-all | grep glib-2.0
  sudo apt-get install libglib2.0-dev
*/
#include "yaml.h"
// definido en yaml.h por lo que no son necesarios
// #include "stdio.h"
#include <pthread.h>
#include <stdio.h>
#include "stdlib.h"
// #include "string.h"
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>


#define YAML_ERROR 0
#define YAML_SUCCESS 1
#define SYSTEM_SUCCESS 0
#define SYSTEM_ERROR 1
#define FILE_ERROR 0
#define ALPHA_SIZE 20
#define FIN_YAML 0
#define NO_FIN_YAML 1
#define MAX_WORD_LENGTH 50
#define MAX_RESPONE_LENGTH 1024
#define MAX_AUTOMATAS 20
  // FORMATS
#define MSG_FORMAT "{ recog: %s, rest: %s }"
#define CODE_MSG_FORMAT "{ codterm: %d, recog: %s, rest: %s }"
#define NODE_MSG_FORMAT "      - node: %s\n        pid: %d\n"

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

typedef enum tags tag;

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

typedef struct automata_desc automata;
typedef struct automata_desc* p_type_automata;
typedef struct nodo_automata nodo;
typedef struct nodo_automata* p_type_nodo;
typedef struct transicion_nodos transicion;
typedef struct transicion_nodos* p_type_transicion;

// definicion de metodos
p_type_automata parseAutomata(yaml_event_t *event, yaml_parser_t *parser);
void parseTransitions(yaml_parser_t *parser, yaml_event_t *event, p_type_nodo *pnodo);
void parseNodesSection(yaml_parser_t *parser, yaml_event_t *event, p_type_automata *pautomata);
// el ... significa que resive atributos dinamicos
// void parseSequenceSection(yaml_event_t *event, yaml_parser_t *parser, int kind, ...);
void parseSequenceSection(yaml_event_t *event, yaml_parser_t *parser, int kind, p_type_automata *pautomata);
int deleteEvent(yaml_event_t *event);
int yamlParser(yaml_event_t *event, yaml_parser_t *parser);
void next(yaml_event_t *event, yaml_parser_t *parser);
void crearHijos(p_type_automata pautomata);
void sendCommand(char *command, char *msg, p_type_automata pautomata);
void yamlInfoNode( char *info, char *id, int ppid ) ;
void printInfoMsg(char *automata_name);

int **fd_padre;

void nodes_printer(p_type_automata pautomata, char *info_msg) {
  p_type_automata aux = pautomata;
  p_type_nodo _n_aux = aux->primer_nodo;
  printInfoMsg(aux->nombre);
  for ( ;_n_aux; _n_aux = _n_aux->next ) {
    yamlInfoNode( info_msg, _n_aux->id, _n_aux->pid);
    fprintf(stdout, "%s", info_msg);
  }
}
// diccionario de TAGS  en el archivo yaml
char *diccionario[20] = {
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

int yamlParser(yaml_event_t *event, yaml_parser_t *parser){
  if( !yaml_parser_parse(parser, event) ) {
     fprintf(stderr,"Parser Error %d\n", (*parser).error);
     fprintf(stderr, "Paser Problem %s\n", (*parser).problem);
     if ((*parser).error == 4) {
        fprintf(stderr, "DOCUMENT_END\n");
     }
     kill( 0, SIGKILL );
  }
  return 1;
}

// si el evento es diferente que fin de archivo elimino el evento para despues seguir
// con el siguiente y retorno 1 si se pudo eliminar para poder continuar, de lo contrario
// retorno 0, es decir que se acabo el yaml
int deleteEvent(yaml_event_t *event) {
  if ( event->type != YAML_STREAM_END_EVENT )
  {
      yaml_event_delete( event );
      return NO_FIN_YAML;
  }
  return FIN_YAML;
}

// pregunto si es fin de archivo, si lo es hubo un erro, sino sigo con el siguiente evento
void next(yaml_event_t *event, yaml_parser_t *parser){
  if( !deleteEvent( event ) )
      exit(EXIT_FAILURE);
  yamlParser(event, parser);
}

void initStringArray(p_type_automata *pautomata) {

  (*pautomata)->alfabeto = (char**) malloc( ALPHA_SIZE * sizeof( char* ) );
  (*pautomata)->estados = (char**) malloc( ALPHA_SIZE * sizeof( char* ) );
  (*pautomata)->final = (char**) malloc( ALPHA_SIZE * sizeof( char* ) );

  int i;
  for (i = 0; i < ALPHA_SIZE; ++i) {
    (*pautomata)->alfabeto[i] = (char*) malloc( MAX_WORD_LENGTH * sizeof( char ) );
    memset((*pautomata)->alfabeto[i], '\0', MAX_WORD_LENGTH);
    /*                      */
    (*pautomata)->estados[i] = (char*) malloc( MAX_WORD_LENGTH * sizeof( char ) );
    memset((*pautomata)->estados[i], '\0', MAX_WORD_LENGTH);
    /*                      */
    (*pautomata)->final[i] = (char*) malloc( MAX_WORD_LENGTH * sizeof( char ) );
    memset((*pautomata)->final[i], '\0', MAX_WORD_LENGTH);
  }
}

char* getCommand( char* data ){
  if ( strcmp(data, diccionario[INFO]) == 0) {
    return diccionario[INFO];
  }else if ( strcmp(data, diccionario[SEND]) == 0) {
    return diccionario[SEND];
  } else if ( strcmp(data, diccionario[STOP]) == 0) {
    sendCommand ( diccionario[STOP], NULL, NULL);
  } else {
    fprintf(stderr, "Error on command\n");
    return NULL;
  }
}

void yamlStringFormater( char *msg, char *recog, char *rest ) {
  sprintf(msg, MSG_FORMAT, recog, rest );
}
void yamlCodeStringFormater ( int codterm, char *msg, char *recog, char *rest ) {
  sprintf(msg, CODE_MSG_FORMAT, codterm, recog, rest );
}

void yamlInfoNode( char *info, char *id, int ppid ) {
  sprintf( info, NODE_MSG_FORMAT, id, ppid );
}

void printInfoMsg(char *automata_name) {
  fprintf(stdout, "- msgtype: %s\n  info:\n    - automata: %s\n      ppid: %d\n", diccionario[INFO], automata_name, getpid());
}

void* controladorHiloLectura(void *args) {
  p_type_automata pautomata = (p_type_automata) args;
  // automata_descPrinter( pautomata );
  char *buffer = (char*) malloc( sizeof(char) * MAX_RESPONE_LENGTH );
  memset( buffer, '\0', MAX_RESPONE_LENGTH);
  while ( 1 ) {
    while ( read( pautomata->pipe_to_father[0], buffer, MAX_RESPONE_LENGTH ) > 0 ) {
      fprintf(stdout, "%s\n", buffer);
    }
  }
}

void sendCommand(char *command, char *msg, p_type_automata pautomata) {
  if ( strcmp(command, diccionario[INFO]) == 0) {
    p_type_automata aux = pautomata;
    p_type_nodo _n_aux;
    char *info_msg = (char*) malloc( sizeof(char) * MAX_WORD_LENGTH );
    memset(info_msg, '\0', MAX_WORD_LENGTH);
    for (; aux; aux = aux->next) {
      if ( strlen(msg) == 0){
        nodes_printer(aux, info_msg);
      } else if ( strcmp(msg, aux->nombre) == 0) {
        nodes_printer(aux, info_msg);
        return;
      }
    }
  } else if ( strcmp(command, diccionario[SEND]) == 0) {
    char *_msg = (char*) malloc( sizeof(msg) * MAX_WORD_LENGTH );
    memset(_msg, '\0', MAX_WORD_LENGTH);
    yamlStringFormater( _msg, "", msg );
    p_type_automata aux = NULL;
    for (aux = pautomata; aux; aux = aux->next) {
      write( aux->primer_nodo->fd[1], _msg, strlen(_msg) );
    }
  } else if ( strcmp(command, diccionario[STOP]) == 0) {
    // 0 = All processes in the same process group as the sender.
    kill( 0, SIGKILL);
  }
}

void startListenUserInput( p_type_automata pautomata) {
  yaml_parser_t parser;
  yaml_event_t event;
  char *data, *command, *msg;
  data = (char*) malloc(sizeof(char) * MAX_WORD_LENGTH);
  memset(data, '\0', MAX_WORD_LENGTH);
  command = (char*) malloc(sizeof(char) * MAX_WORD_LENGTH);
  memset(command, '\0', MAX_WORD_LENGTH);
  msg = (char*) malloc(sizeof(char) * MAX_WORD_LENGTH);
  memset( msg, '\0', MAX_WORD_LENGTH );
  while( 1 ) {
    if ( !yaml_parser_initialize( &parser ) ){
      fprintf(stderr, "Unable to initialize yaml parser\n");
      exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Entre mensaje:\n\t$: ");
    fgets(msg, MAX_WORD_LENGTH, stdin);
    size_t size = strlen(msg) - 1;
    if (msg[size] == '\n'){
      msg[size] = '\0';
    }
    yaml_parser_set_input_string( &parser, msg, size );
    if (yamlParser( &event,&parser )) {
      while (event.type != YAML_STREAM_END_EVENT) {
        if (event.type != YAML_SCALAR_EVENT ) {
          next(&event, &parser);
          continue;
        } else {
          strcpy( data, event.data.scalar.value );
          if ( strcmp( data, diccionario[CMD] ) == 0 ) {
            next(&event,&parser);
            if ( event.type == YAML_SCALAR_EVENT ) {
              strcpy( data, event.data.scalar.value );
              command = getCommand(data );
              if ( !command ) {
                fprintf(stderr, "Command Not Found\n");
                break;
              }
              next(&event,&parser);
           }
         } else if ( strcmp( data, diccionario[MSG] ) == 0 ){
            next(&event, &parser);
            if ( event.type == YAML_SCALAR_EVENT ) {
              strcpy( data, event.data.scalar.value );
              sendCommand( command, data, pautomata );
            }
          } else {
            break;
          }
        }
      }
    }
    yaml_parser_delete( &parser );
  }
}

void controladorProcesos(p_type_nodo *pnodo, char* nombre_automata, char **estados_finales, int size_finales/*, int *fd*/) {
    char *buffer = (char *) malloc( sizeof(char) * MAX_RESPONE_LENGTH);
    memset(buffer, '\0', MAX_WORD_LENGTH);
    char *recog;
    char *data = (char*) malloc( sizeof( char ) * MAX_WORD_LENGTH );
    char *_msg = (char*) malloc( sizeof( char ) * MAX_WORD_LENGTH );
    memset(_msg, '\0', MAX_WORD_LENGTH);
    int send = 0, check = 0;
    while(1)
    {
      while (read((*pnodo)->fd[0], buffer, MAX_WORD_LENGTH) > 0) {
        _msg[0] = '\0';
        yaml_parser_t parser;
        yaml_event_t event;
        send = 0;
        check = 0;
        if ( !yaml_parser_initialize( &parser ) ){
            fprintf(stderr, "Unable to initialize yaml parser\n");
            exit(EXIT_FAILURE);
        }
        yaml_parser_set_input_string( &parser, buffer, strlen( buffer + 1 ) );
        if ( yamlParser( &event, &parser ) ){
            while (event.type != YAML_STREAM_END_EVENT) {
                if (event.type != YAML_SCALAR_EVENT ) {
                    next(&event, &parser);
                    continue;
                } else {
                  strcpy( data, event.data.scalar.value );
                  if ( strcmp( data, diccionario[RECOG] ) == 0 ) {
                      next(&event,&parser);
                      if ( event.type == YAML_SCALAR_EVENT ) {
                          recog = (char*) malloc( sizeof(char) * strlen( event.data.scalar.value ) + 1);
                          strcpy( recog, event.data.scalar.value );
                          next(&event,&parser);
                     }
                 } else if ( strcmp( data, diccionario[REST] ) == 0 ){
                      next(&event, &parser);
                      if ( event.type == YAML_SCALAR_EVENT ) {
                          strcpy( data, event.data.scalar.value );
                      }
                  } else { break ;}
                }
            }
        }
        p_type_transicion aux = (*pnodo)->primer_transicion;
        for(; aux; aux = aux->next) {
            if ( memcmp(aux->entrada, data, strlen(aux->entrada) ) == 0 ) {
                strncat(recog, data, strlen(aux->entrada));
                data+= strlen(aux->entrada);
                yamlStringFormater(_msg, recog, data);
                p_type_nodo pnodo_aux = (*pnodo)->primer_nodo;
                for (; pnodo_aux; pnodo_aux = pnodo_aux->next) {
                    if ( strcmp( aux->sig_estado, pnodo_aux->id) == 0 ) {
                        write(pnodo_aux->fd[1], _msg, strlen(_msg));
                        send = 1;
                        break;
                    }
                }
                break;
            }
        }
        if ( !send ) {
          int i = 0;
          for (i; i < size_finales; ++i) {
            if ( strcmp( estados_finales[i], (*pnodo)->id ) == 0 && strlen(data) == 0 ) {
              yamlCodeStringFormater(0, _msg, recog, data);
              check = 1;
              break;
            }
          }
          if ( !check ) {
            yamlCodeStringFormater(1, _msg, recog, data);
          }
          write( (*pnodo)->pipe_to_father[1], _msg, strlen(_msg) + 1 );
        }
      }
    }
}

int creadorProcesoPorNodo(p_type_nodo pnodo, char* nombre_automata, int nodo, char **estados_finales, int size_finales) {
  p_type_nodo aux = NULL;
  for( aux = pnodo; aux; aux = aux->next) {
    if (pipe(fd_padre[nodo]) == -1) {
       perror("pipe");
       exit(EXIT_FAILURE);
     }
    aux->fd = fd_padre[nodo];
    ++nodo;
  }
  pid_t id;
  for( aux = pnodo; aux; aux = aux->next) {
    if ( (id = fork()) == 0 ) {
      // dentro del proceso hijo
      // fprintf(stdout, "Ejecutando el hijo: con id => %d\n",getpid());
      controladorProcesos( &aux, nombre_automata, estados_finales, size_finales/*, fd_padre[nodo] */);
      break;
    }else{
      // dentro del padre
      aux->pid = id;
      // fprintf(stdout, "Dentro del padre: id => %d\n", getpid());
    }
  }
  return nodo;
}

void crearHijos( p_type_automata pautomata) {
  p_type_automata aux = NULL;
  int i = 0;
  fd_padre = (int**) malloc(MAX_AUTOMATAS * sizeof(int*));
  for ( i; i < MAX_AUTOMATAS; ++i) {
    fd_padre[i] = (int*) malloc( sizeof(int) * 2);
  }
  int nodo = 0;
  for (aux = (pautomata); aux; aux = aux->next) {
    pthread_create(&(aux->hilo_lectura), NULL, controladorHiloLectura, (void *) aux);
    nodo = creadorProcesoPorNodo(aux->primer_nodo, aux->nombre, nodo, aux->final, aux->sizeFinal);
  }
  startListenUserInput( pautomata );
}

void parseTransitions(yaml_parser_t *parser, yaml_event_t *event, p_type_nodo *pnodo){
  next(event, parser);
  char *data = (char *)malloc(sizeof(char) * MAX_WORD_LENGTH);
  strcpy(data, event->data.scalar.value);
  if ( strcmp( data, diccionario[TRANS]) != 0 )
      exit(EXIT_FAILURE);
  next(event, parser);
  (*pnodo)->transiciones = NULL;
  if (event->type != YAML_SEQUENCE_START_EVENT) {
    /* code */
    // fprintf(stdout, "Parece ser un estado final\n0 Transiciones\n");
    next(event, parser);
    return;
  }
  next(event, parser);
  while( event->type == YAML_MAPPING_START_EVENT ) {
    next(event, parser);
    strcpy(data, event->data.scalar.value);
    if ( strcmp( data, diccionario[IN]) == 0 ) {
      p_type_transicion ptransicion = ( p_type_transicion) malloc( sizeof( transicion ) );
      ptransicion->entrada = malloc( sizeof(char) * MAX_WORD_LENGTH );
      ptransicion->sig_estado = malloc( sizeof(char) * MAX_WORD_LENGTH );
      // pedir proximo evento y asignarselo a la entrada
      next(event, parser);
      strcpy(data, event->data.scalar.value);
      strcpy( ptransicion->entrada, data );
      // el proximo tag es next que nos indica el estado para donde vamos
      next(event, parser);
      // pedir proximo evento y asignarselo a el siguiendo estado
      next(event, parser);
      strcpy(data, event->data.scalar.value);
      strcpy( ptransicion->sig_estado, data );
      // (*pnodo)->transiciones = g_slist_append( (*pnodo)->transiciones, ptransicion );
      if ((*pnodo)->transiciones == NULL){
        (*pnodo)->primer_transicion = ptransicion;
        (*pnodo)->transiciones = ptransicion;
      } else {
        (*pnodo)->transiciones->next = ptransicion;
        (*pnodo)->transiciones = ptransicion;
        ptransicion = ptransicion->next;
      }
      // volver a pedir el proximo evento para poder ver si es o no otra transicion
      next(event, parser);
      if (event->type == YAML_MAPPING_END_EVENT) {
        /* code */
        next(event, parser);
      }
    }
  }
}

void parseNodesSection(yaml_parser_t *parser, yaml_event_t *event, p_type_automata *pautomata){
  next(event, parser);
  char *data = (char *)malloc(sizeof(char) * MAX_WORD_LENGTH);
  int size = 0;
  strcpy(data, event->data.scalar.value);
  if ( strcmp(data, diccionario[DELTA]) == 0 ) {
        next(event, parser);
        if (event->type == YAML_SEQUENCE_START_EVENT){
              next(event, parser);
        }else{
              exit(EXIT_FAILURE);
        }
      p_type_nodo pnodo = NULL;
      (*pautomata)->nodos = NULL;
      while (event->type == YAML_MAPPING_START_EVENT){
            next(event, parser);
            strcpy(data, event->data.scalar.value);
            if ( strcmp( data, diccionario[NODE]) == 0){
                  next(event, parser);
                  pnodo = (p_type_nodo) malloc( sizeof( nodo ) );
                  pnodo->id = (char *) malloc( sizeof(char) * MAX_WORD_LENGTH );
                  strcpy(pnodo->id, event->data.scalar.value);
                  pnodo->pipe_to_father = (int*) malloc( sizeof(int) * 2);
                  pnodo->pipe_to_father = (*pautomata)->pipe_to_father;
                  parseTransitions(parser, event, &pnodo);
                  if( (*pautomata)->nodos == NULL){
                        (*pautomata)->nodos = pnodo;
                        (*pautomata)->primer_nodo = pnodo;
                  }else{
                        (*pautomata)->nodos->next = pnodo;
                        (*pautomata)->nodos = pnodo;
                  }
                  (*pautomata)->nodos->primer_nodo = (*pautomata)->primer_nodo;
                  pnodo = pnodo->next;
                  if( event->type == YAML_SEQUENCE_END_EVENT){
                        next(event, parser);
                        if( event->type == YAML_MAPPING_END_EVENT){
                          next( event, parser );
                        }
                  }
                  ++size;
            }
      }
  }
}

// void parseSequenceSection(yaml_event_t *event, yaml_parser_t *parser, int kind, ...){
void parseSequenceSection(yaml_event_t *event, yaml_parser_t *parser, int kind, p_type_automata *pautomata){
    // lista de argumentos dinamicos
    // va_list args;
    // inicializar lista de argumentos con los elementos que vayan despues de kind
    // va_start ( args, kind );
    char *data = (char*) malloc(sizeof(char) * MAX_WORD_LENGTH);
    switch(kind)
    {
      case ALPHA:
      { //scope for ALPHA case http://stackoverflow.com/questions/92396/why-cant-variables-be-declared-in-a-switch-statement
          // sacar el elemento de la listo suponiendo que tiene que del tipo del segundo argumento
          // p_type_automata pautomata = va_arg( args, p_type_automata );
          // fprintf(stdout, "in SEQUENCE ALPHA Section\n");
          int pos_alfab = 0;
          next(event, parser);
          while(event->type == YAML_SCALAR_EVENT) {
              strcpy(data, event->data.scalar.value);
              // fprintf(stdout, "\t%s\n", data);
              (*pautomata)->alfabeto[pos_alfab] = (char *) malloc(event->data.scalar.length);
              strcpy((*pautomata)->alfabeto[pos_alfab], data);
              ++pos_alfab;
              next(event, parser);
          }
          if( !pos_alfab)
              exit(EXIT_FAILURE);
          (*pautomata)->sizeAlfabeto = pos_alfab;
          break;
      }
      case STATES:
      { //scope for STATES case
          // p_type_nodo pnodo = va_arg( args, p_type_nodo );
          // sacar el elemento de la lista suponiendo que tiene que del tipo del segundo argumento
          // p_type_automata pautomata = va_arg( args, p_type_automata );
          // fprintf(stdout, "in SEQUENCE STATES Section\n");
          int pos_alfab = 0;
          next(event, parser);
          while(event->type == YAML_SCALAR_EVENT) {
              strcpy(data, event->data.scalar.value);
              // fprintf(stdout, "\t%s\n", data);
              (*pautomata)->estados[pos_alfab] = (char *) malloc(event->data.scalar.length);
              strcpy((*pautomata)->estados[pos_alfab], data);
              ++pos_alfab;
              next(event, parser);
          }
          if( !pos_alfab)
              exit(EXIT_FAILURE);
          (*pautomata)->sizeEstados = pos_alfab;
          break;
      }
      case FINAL:
      {
        // p_type_automata pautomata = va_arg( args, p_type_automata );
        // fprintf(stdout, "in SEQUENCE FINAL Section\n");
        int pos_alfab = 0;
        next(event, parser);
        while(event->type == YAML_SCALAR_EVENT) {
            strcpy(data, event->data.scalar.value);
            // fprintf(stdout, "\t%s\n", data);
            (*pautomata)->final[pos_alfab] = (char *) malloc(event->data.scalar.length);
            strcpy((*pautomata)->final[pos_alfab], data);
            ++pos_alfab;
            next(event, parser);
        }
        if( !pos_alfab)
            exit(EXIT_FAILURE);
        // fprintf(stdout,"%d\n", event->type);
        (*pautomata)->sizeFinal = pos_alfab;
        // fprintf(stdout,"%d\n", event->type);
        break;
      }
    }
    // va_end ( args );
}

p_type_automata parseAutomata(yaml_event_t *event, yaml_parser_t *parser) {
  char *data = (char *)malloc(sizeof(char) * MAX_WORD_LENGTH);
  strcpy(data, event->data.scalar.value);
  p_type_automata pautomata = (p_type_automata)  malloc( sizeof( automata ) );
  initStringArray(&pautomata);
  // si el dato es = automata pido el siguiente evento que debe de ser el nombre del automata
  // fprintf(stdout, "AUTOMATA\n");
  if (strcmp( data, diccionario[AUTOMATA] ) == 0) {
        /* code */
        // if( !deleteEvent(event) )
        //   exit(EXIT_FAILURE);
        // yamlParser(event, parser);
        next(event, parser);
        pautomata->nombre = (char*) malloc(event->data.scalar.length);
        strcpy(pautomata->nombre, event->data.scalar.value);
        // fprintf(stdout, "\t%s\n", pautomata->nombre);
  }
  // el siguiente evento debe de ser la descripcion
  next(event, parser);
  strcpy(data, event->data.scalar.value);
  // fprintf(stdout, "DESCRIPTION\n");
  if (strcmp( data, diccionario[DESCRIPTION] ) == 0) {
        /* code */
        next(event, parser);
        pautomata->descripcion = (char*) malloc(event->data.scalar.length);
        strcpy(pautomata->descripcion, event->data.scalar.value);
        // fprintf(stdout, "\t%s\n", pautomata->descripcion);
  }
  // el siguiente evento debe de ser el alfabeto, el cual contiene una secuencia donde
  // los caracteres estan encerrados en forma de arreglo
  next(event, parser);
  strcpy(data, event->data.scalar.value);
  // fprintf(stdout, "ALPHA\n");
  if (strcmp( data, diccionario[ALPHA] ) == 0) {
        /* code */
        // el siguiente evento debe de ser una secuencia de letras que componen al alfabeto
        next(event, parser);
        switch (event->type)
        {
          /* code */
          case YAML_SEQUENCE_START_EVENT:
              // parseo esa secuencia la cual se ve como una seccion
              parseSequenceSection(event, parser, ALPHA, &pautomata);
              // luego de haber parseado la secuencia debe de seguir fin de secuencia, sino
              // es porque hay un error
              if (event->type != YAML_SEQUENCE_END_EVENT)
                exit(EXIT_FAILURE);
              break;
        }
  }
  // el siguiente evento tiene que ser los estados que tendra el automata, que tambien
  // poseen una secuencia donde tiene todos los estados como letras, leer alfabeto
  next(event, parser);
  strcpy(data, event->data.scalar.value);
  // fprintf(stdout, "STATES\n");
  if (strcmp( data, diccionario[STATES] ) == 0) {
        /* code */
        next(event, parser);
        switch (event->type)
        {
          /* code */
          case YAML_SEQUENCE_START_EVENT:
              parseSequenceSection(event, parser, STATES, &pautomata);
              if (event->type != YAML_SEQUENCE_END_EVENT)
                exit(EXIT_FAILURE);
              break;
        }
  }
  next(event, parser);
  strcpy(data, event->data.scalar.value);
  // fprintf(stdout, "START\n");
  if (strcmp( data, diccionario[START] ) == 0) {
    /* code */
    next(event, parser);
    // fprintf(stdout, "\t%s\n",  event->data.scalar.value);
    pautomata->estadoinicial = (char *) malloc(sizeof(char) * MAX_WORD_LENGTH);
    strcpy(pautomata->estadoinicial, event->data.scalar.value);
    // fprintf(stdout, "\t%s\n", pautomata->estadoinicial);
  }
  next(event, parser);
  strcpy(data, event->data.scalar.value);
  // fprintf(stdout, "FINAL\n");
  if (strcmp( data, diccionario[FINAL] ) == 0) {
        /* code */
        next(event, parser);
        switch (event->type)
        {
          /* code */
          case YAML_SEQUENCE_START_EVENT:
              parseSequenceSection(event, parser, FINAL, &pautomata);
              if (event->type != YAML_SEQUENCE_END_EVENT)
                exit(EXIT_FAILURE);
              break;
        }
  }
  pautomata->pipe_to_father = (int*) malloc( sizeof(int) * 2);
   if ( pipe(pautomata->pipe_to_father) == -1 ) {
    perror("Error creating pipe_to_father");
    exit(EXIT_FAILURE);
   }
  parseNodesSection(parser, event, &pautomata);
  return pautomata;
}

p_type_automata startParsingYamlFile(yaml_parser_t *parser) {
  yaml_event_t event;
  // contadores para estipular si concuerdan el numero de tags que abren y cierran
  // las determinadas secciones
  p_type_automata pautomata = NULL;
  p_type_automata primer_pautomata = NULL;
  do
  {
      // if( yaml_parser_parse(&parser, &event) == 0 )
      // yamlParser(event, *parser);
      if( !yaml_parser_parse(parser, &event) )
      {
         fprintf(stderr, "Parser Error %d\n", (*parser).error);
         exit(EXIT_FAILURE);
      }
      // printf("EVENT TYPE: %d\n",(event).type);
      switch((event).type)
      {
          case YAML_SCALAR_EVENT:
              // fprintf(stdout, "YAML VALUE: %s\n", event.data.scalar.value);
              if (pautomata == NULL){
                  primer_pautomata = parseAutomata( &event, parser );
                  pautomata = primer_pautomata;
              }else {
                  pautomata->next = parseAutomata( &event, parser );
                  pautomata = pautomata->next;
              }
              break;
      }
      deleteEvent(&event);
      // if ( event->type != YAML_STREAM_END_EVENT )
      // {
      //     yaml_event_delete( &event );
      // }
  } while ((event).type != YAML_STREAM_END_TOKEN);
  yaml_event_delete( &event );
  // clean parser
  return primer_pautomata;
}

int main(int argc, char const *argv[]){
  setpgid( getpid(), getpid() );
  if ( argc < 2 )
  {
    printf("There is no file to parse, try again\n");
    return SYSTEM_ERROR;
  }
  FILE *yaml_file = fopen( argv[1], "r" );
  if ( yaml_file == FILE_ERROR )
  {
    printf("Could not open file\n");
    return SYSTEM_ERROR;
  }
  yaml_parser_t parser;

  // ini yaml parser
  // if ( yaml_parser_initialize( &parser ) == YAML_ERROR)
  if ( !yaml_parser_initialize( &parser ) )
    printf("Unable to initialize yaml parser\n");
  //read yaml file
  yaml_parser_set_input_file( &parser, yaml_file );

  // printf("Start parsing yaml file\n");

  p_type_automata pautomata = NULL;
  pautomata = startParsingYamlFile(&parser);
  // printf("Already parsed\n");
  yaml_parser_delete( &parser );
  fclose( yaml_file );

  crearHijos(pautomata);
  return SYSTEM_SUCCESS;
}
