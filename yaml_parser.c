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
#include <stdio.h>
#include <stdio_ext.h>
#include "stdlib.h"
// #include "string.h"
#include <unistd.h>
#include "glib.h"
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>


#define YAML_ERROR 0
#define YAML_SUCCESS 1
#define SYSTEM_SUCCESS 0
#define SYSTEM_ERROR 1
#define FILE_ERROR 0
#define ALPHA_NUMBER 200
#define FIN_YAML 0
#define NO_FIN_YAML 1
#define MAX_WORD_LENGTH 50
#define MAX_AUTOMATAS 20

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
  STOP
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
    struct transicion_nodos *transiciones;
    struct transicion_nodos *primer_transicion;
    struct nodo_automata *next;
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
    // GSList* nodos;
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
void startSequence();
void startMapping();
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

int **fd_padre;

void automata_descPrinter(p_type_automata pautomata){
  fprintf(stdout, "Nombre Automata: %s\n", (pautomata->nombre));
  fprintf(stdout, "Descripcion: %s\n", (pautomata->descripcion));
  // array of strings
  fprintf(stdout, "Alfabeto: \n");
  int i = 0;
  while(i < pautomata->sizeAlfabeto){
    fprintf(stdout, ":%s\t\n",(pautomata->alfabeto[i]));
    ++i;
  }
  i = 0;
  // array of strings
  fprintf(stdout, "Estados: \n");
  while(i < pautomata->sizeEstados){
    fprintf(stdout, ":%s\t\n",(pautomata->estados[i]));
    ++i;
  }
  i = 0;
  fprintf(stdout, "Estado Inicial: %s\n", (pautomata->estadoinicial));
  // array of strings
  fprintf(stdout, "Estados Finales: \n");
  while(i < pautomata->sizeFinal){
    fprintf(stdout, ":%s\t\n",(pautomata->final[i]));
    ++i;
  }
}

void nodes_printer(p_type_automata pautomata) {
  p_type_nodo aux = NULL;
  for (aux = pautomata->primer_nodo; aux; aux= aux->next) {
    fprintf(stdout, "Nodo: %s\n", aux->id);
    fprintf(stdout, "Transiciones:\n");
    p_type_transicion t_aux = NULL;
    for (t_aux = aux->primer_transicion; t_aux; t_aux = t_aux->next) {
      fprintf(stdout, "En:\t%s\n", t_aux->entrada);
      fprintf(stdout, "Siguiente Estado:\t%s\n", t_aux->sig_estado);
    }
  }
}
// diccionario de TAGS  en el archivo yaml
char *diccionario[17] = {
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
  "stop"
};

int yamlParser(yaml_event_t *event, yaml_parser_t *parser){
  if( !yaml_parser_parse(parser, event) )
  {
     fprintf(stderr,"Parser Error %d\n", (*parser).error);
     fprintf(stderr, "Paser Problem %s\n", (*parser).problem);
     if ((*parser).error == 4) {
        fprintf(stderr, "DOCUMENT_END\n");
     }
     kill( 0, SIGKILL );
     // exit(EXIT_FAILURE);
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

void initStringArray(p_type_automata *pautomata, int arrayLength, int wordLength) {

  // if (!arrStr) {
  //   fprintf(stderr,
   //    "yaml_parser.c:initStringArray:Error: the input is not null\n");
  //   return NULL;
  // }

  (*pautomata)->alfabeto = (char**) malloc( arrayLength * sizeof( char* ) );
  (*pautomata)->estados = (char**) malloc( arrayLength * sizeof( char* ) );
  (*pautomata)->final = (char**) malloc( arrayLength * sizeof( char* ) );

  int i;
  for (i = 0; i < arrayLength; ++i) {
    (*pautomata)->alfabeto[i] = (char*) malloc( wordLength * sizeof( char ) );
    memset((*pautomata)->alfabeto[i], '\0', MAX_WORD_LENGTH);
    /*                      */
    (*pautomata)->estados[i] = (char*) malloc( wordLength * sizeof( char ) );
    memset((*pautomata)->estados[i], '\0', MAX_WORD_LENGTH);
    /*                      */
    (*pautomata)->final[i] = (char*) malloc( wordLength * sizeof( char ) );
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

void sendCommand(char *command, char *msg, p_type_automata pautomata) {
  if ( strcmp(command, diccionario[INFO]) == 0) {
    p_type_automata aux = pautomata;
    for (; aux; aux = aux->next) {
      if ( strlen(msg) == 0 ) {
        write( aux->primer_nodo->fd[1], diccionario[INFO], strlen( diccionario[INFO] ) );
      } else if ( strcmp( aux->nombre, msg ) == 0 ) {
        write( aux->primer_nodo->fd[1], diccionario[INFO], strlen( diccionario[INFO] ) );
        return;
      }
    }
  } else if ( strcmp(command, diccionario[SEND]) == 0) {
    p_type_automata aux = NULL;
    int nodos = 0;
    // int old_fd = dup(0);
    // close(0);
    for (aux = pautomata; aux; aux = aux->next) {
      // close(fd_padre[nodos % aux->sizeEstados][0]);
      // close(fd_padre[nodos % aux->sizeEstados][1]);
      write(
        //fd_padre[nodos/*% aux->sizeEstados*/][1],
        aux->primer_nodo->fd[1],
        msg,
        strlen(msg)
      );
      close(fd_padre[nodos /*% aux->sizeEstados*/][1]);
      // dup2(0, old_fd);
      nodos += aux->sizeEstados;
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
    // fflush(stdin);
    // __fpurge(stdin);
    // fgets(msg, MAX_WORD_LENGTH, stdin);
    gets(msg);
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
              fprintf(stdout, "Sending: %s\n", data);
              sendCommand( command, data, pautomata );
            }
          } else {
            break;
          }
        }
      }
    }
    yaml_parser_delete( &parser );
    // msg[0] = '\0';
  }
}

void controladorProcesos(p_type_nodo *pnodo, pid_t hijo, char* nombre_automata/*, int *fd*/) {
    // fprintf(stdout, "Ejecutando el Metodo del hijo: con id => %d\n",getpid());
    // fprintf(stdout, "Ejecutando el Metodo del hijo enviado: con id => %d\n",hijo);
    fprintf(stdout, "%d: Nodo: %s Con automata: %s\n", hijo, (*pnodo)->id, nombre_automata);
    char *buffer = (char *) malloc( sizeof(char) * MAX_WORD_LENGTH);
    memset(buffer, '\0', MAX_WORD_LENGTH);
    while(1)
    {
      // close stdin read
      // close(0);
      // coja el pipe de entrada del padre y pegueselo al proceso
      // dup2(0, (*pnodo)->fd[0]);
      // cierre la salida del pipe write
      // close((*pnodo)->fd[1]);
      // start listen for events

      while (read((*pnodo)->fd[0], buffer, MAX_WORD_LENGTH) > 0) {
        fprintf(stdout, "in Process %s in Automata[%s] msg was %s\n", (*pnodo)->id, nombre_automata, buffer);
        // if ( strcmp( buffer, diccionario[INFO]  ) == 0 ) {
        //   if ( (*pnodo)->next ) {
        //     dup2(  (*pnodo)->next->fd[0], 0);
        //     write( (*pnodo)->next->fd[1], diccionario[INFO], strlen( diccionario[INFO] ));
        //   }
        // }
      }
    }
}

int controladorNodos(p_type_nodo pnodo, char* nombre_automata, int nodo) {
  p_type_nodo aux = NULL;
  for( aux = pnodo; aux; aux = aux->next) {
    if (pipe(fd_padre[nodo]) == -1) {
       perror("pipe");
       exit(EXIT_FAILURE);
     }
    aux->fd = fd_padre[nodo];
    ++nodo;
    if ( fork() == 0 ) {
      // dentro del proceso hijo
      // fprintf(stdout, "Ejecutando el hijo: con id => %d\n",getpid());
      controladorProcesos( &aux, getpid(), nombre_automata/*, fd_padre[nodo] */);
      break;
    }else{
      // dentro del padre
      fprintf(stdout, "Dentro del padre: id => %d\n", getpid());
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
  for (aux = pautomata; aux; aux = aux->next) {
    nodo = controladorNodos(aux->primer_nodo, aux->nombre, nodo);
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
    // exit(EXIT_FAILURE);
    fprintf(stdout, "Parece ser un estado final\n0 Transiciones\n");
    next(event, parser);
    // if (event->type == YAML_MAPPING_END_EVENT)
    //   next(event, parser);
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
                  parseTransitions(parser, event, &pnodo);
                  if( (*pautomata)->nodos == NULL){
                        (*pautomata)->nodos = pnodo;
                        (*pautomata)->primer_nodo = pnodo;
                  }else{
                        (*pautomata)->nodos->next = pnodo;
                        (*pautomata)->nodos = pnodo;
                  }
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
          fprintf(stdout, "in SEQUENCE ALPHA Section\n");
          int pos_alfab = 0;
          next(event, parser);
          while(event->type == YAML_SCALAR_EVENT) {
              strcpy(data, event->data.scalar.value);
              fprintf(stdout, "\t%s\n", data);
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
          fprintf(stdout, "in SEQUENCE STATES Section\n");
          int pos_alfab = 0;
          next(event, parser);
          while(event->type == YAML_SCALAR_EVENT) {
              strcpy(data, event->data.scalar.value);
              fprintf(stdout, "\t%s\n", data);
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
        fprintf(stdout, "in SEQUENCE FINAL Section\n");
        int pos_alfab = 0;
        next(event, parser);
        while(event->type == YAML_SCALAR_EVENT) {
            strcpy(data, event->data.scalar.value);
            fprintf(stdout, "\t%s\n", data);
            (*pautomata)->final[pos_alfab] = (char *) malloc(event->data.scalar.length);
            strcpy((*pautomata)->final[pos_alfab], data);
            ++pos_alfab;
            next(event, parser);
        }
        if( !pos_alfab)
            exit(EXIT_FAILURE);
        fprintf(stdout,"%d\n", event->type);
        (*pautomata)->sizeFinal = pos_alfab;
        fprintf(stdout,"%d\n", event->type);
        break;
      }
    }
    // va_end ( args );
}

p_type_automata parseAutomata(yaml_event_t *event, yaml_parser_t *parser) {
  char *data = (char *)malloc(sizeof(char) * MAX_WORD_LENGTH);
  strcpy(data, event->data.scalar.value);
  p_type_automata pautomata = (p_type_automata)  malloc( sizeof( automata ) );
  initStringArray(&pautomata, ALPHA_NUMBER, MAX_WORD_LENGTH);
  // si el dato es = automata pido el siguiente evento que debe de ser el nombre del automata
  fprintf(stdout, "AUTOMATA\n");
  if (strcmp( data, diccionario[AUTOMATA] ) == 0) {
        /* code */
        // if( !deleteEvent(event) )
        //   exit(EXIT_FAILURE);
        // yamlParser(event, parser);
        next(event, parser);
        pautomata->nombre = (char*) malloc(event->data.scalar.length);
        strcpy(pautomata->nombre, event->data.scalar.value);
        fprintf(stdout, "\t%s\n", pautomata->nombre);
  }
  // el siguiente evento debe de ser la descripcion
  next(event, parser);
  strcpy(data, event->data.scalar.value);
  fprintf(stdout, "DESCRIPTION\n");
  if (strcmp( data, diccionario[DESCRIPTION] ) == 0) {
        /* code */
        next(event, parser);
        pautomata->descripcion = (char*) malloc(event->data.scalar.length);
        strcpy(pautomata->descripcion, event->data.scalar.value);
        fprintf(stdout, "\t%s\n", pautomata->descripcion);
  }
  // el siguiente evento debe de ser el alfabeto, el cual contiene una secuencia donde
  // los caracteres estan encerrados en forma de arreglo
  next(event, parser);
  strcpy(data, event->data.scalar.value);
  fprintf(stdout, "ALPHA\n");
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
  fprintf(stdout, "STATES\n");
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
  fprintf(stdout, "START\n");
  if (strcmp( data, diccionario[START] ) == 0) {
    /* code */
    next(event, parser);
    // fprintf(stdout, "\t%s\n",  event->data.scalar.value);
    pautomata->estadoinicial = (char *) malloc(sizeof(char) * MAX_WORD_LENGTH);
    strcpy(pautomata->estadoinicial, event->data.scalar.value);
    fprintf(stdout, "\t%s\n", pautomata->estadoinicial);
  }
  next(event, parser);
  strcpy(data, event->data.scalar.value);
  fprintf(stdout, "FINAL\n");
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
  parseNodesSection(parser, event, &pautomata);
  // fprintf(stdout, "============================================\n");
  // fprintf(stdout, "Nodes Section print\n");
  // automata_descPrinter(pautomata);
  // nodes_printer(pautomata);
  // fprintf(stdout, "Nodes Section After print\n");
  // fprintf(stdout, "============================================\n");
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
      printf("EVENT TYPE: %d\n",(event).type);
      switch((event).type)
      {
          case YAML_SCALAR_EVENT:
              fprintf(stdout, "YAML VALUE: %s\n", event.data.scalar.value);
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

  printf("Start parsing yaml file\n");

  p_type_automata pautomata = NULL;
  pautomata = startParsingYamlFile(&parser);
  printf("Already parsed\n");
  yaml_parser_delete( &parser );
  fclose( yaml_file );

  crearHijos(pautomata);
  return SYSTEM_SUCCESS;
}
