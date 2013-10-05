/*
  USAR cc -o name yaml_parser.c -lyaml
  LUEGO ./name file.yaml

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
// #include "stdlib.h"
// #include "string.h"
#include "unistd.h"
#include "glib.h"
// argumentos dinamicos
#include "stdarg.h"

#define YAML_ERROR 0
#define YAML_SUCCESS 1
#define SYSTEM_SUCCESS 0
#define SYSTEM_ERROR 1
#define FILE_ERROR 0
#define ALPHA_NUMBER 200
#define STATES_NUMBER 200
#define FIN_YAML 0
#define NO_FIN_YAML 1
#define MAX_WORD_LENGTH 50

/*
  #define AUTOMATA 0
  #define DESCRIPTION 1
  #define ALPHA 2
  #define STATES 3
  #define START 4
  #define FINAL 5
  #define DELTA 6
  #define NODE 7
  #define TRANS 8
  #define IN 9
  #define NEXT 10
*/

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
  NEXT
};

typedef enum tags tag;



struct flujo_nodos{
    char *entrada;
    char sig_estado;
};

struct transicion_nodos { 
    GSList* flujo_nodos;
};

struct nodo_automata{
    char id;
    GSList* transicion_nodos;
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
    GSList* nodos_automata;
};

typedef struct automata_desc automata;
typedef struct automata_desc* p_type_automata;
typedef struct nodo_automata nodo;
typedef struct nodo_automata* p_type_nodo;
typedef struct flujo_nodos flujo;
typedef struct flujo_nodos* p_type_flujo;
typedef struct transicion_nodos transicion;
typedef struct transicion_nodos* p_type_transicion;

// definicion de metodos
void startSequence();
void startMapping();
// void parseDescriptionSection(yaml_parser_t parser, yaml_event_t event);
void parseDescriptionSection(yaml_event_t *event, yaml_parser_t *parser);
// el ... significa que resive atributos dinamicos
// void parseSequenceSection(yaml_event_t *event, yaml_parser_t *parser, int kind, ...);
void parseSequenceSection(yaml_event_t *event, yaml_parser_t *parser, int kind, p_type_automata *pautomata);
int deleteEvent(yaml_event_t *event);
void yamlParser(yaml_event_t *event, yaml_parser_t *parser);
void next(yaml_event_t *event, yaml_parser_t *parser);

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

// diccionario de TAGS  en el archivo yaml
char *diccionario[11] = { 
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
  "next"
};

void yamlParser(yaml_event_t *event, yaml_parser_t *parser){
  if( !yaml_parser_parse(parser, event) )
  {
     printf("Parser Error %d\n", (*parser).error);
     exit(EXIT_FAILURE);
  }
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
    (*pautomata)->estados[i] = (char*) malloc( wordLength * sizeof( char ) );
    (*pautomata)->final[i] = (char*) malloc( wordLength * sizeof( char ) );
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
          (*pautomata)->sizeAlfabeto = pos_alfab + 1;
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
          (*pautomata)->sizeEstados = pos_alfab + 1;
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
        (*pautomata)->sizeFinal = pos_alfab + 1;
        fprintf(stdout,"%d\n", event->type);
        break;
      }
    }
    // va_end ( args );
}

void parseDescriptionSection(yaml_event_t *event, yaml_parser_t *parser) {
  char *data = (char *)malloc(sizeof(char) * MAX_WORD_LENGTH);
  strcpy(data, event->data.scalar.value);
  p_type_automata pautomata = (p_type_automata)  malloc( sizeof( automata ) );
  // pautomata->alfabeto = (char **)malloc(sizeof(char)*STATES_NUMBER);
  // pautomata->estados = (char **)malloc(sizeof(char)*STATES_NUMBER);
  // pautomata->final = (char **)malloc(sizeof(char)*STATES_NUMBER);
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
  fprintf(stdout, "============================================\n");
  fprintf(stdout, "Before print\n");
  automata_descPrinter(pautomata);
  fprintf(stdout, "After print\n");
}

void startParsingYamlFile(yaml_parser_t *parser) {
  yaml_event_t event;
  // contadores para estipular si concuerdan el numero de tags que abren y cierran
  // las determinadas secciones
  int sequenceCounter = 0;
  int mappingCounter = 0;
  int streamCounter = 0;
  int documentCounter = 0;
  do
  {
      // if( yaml_parser_parse(&parser, &event) == 0 )
      // yamlParser(event, *parser);
      if( !yaml_parser_parse(parser, &event) )
      {
         printf("Parser Error %d\n", (*parser).error);
         exit(EXIT_FAILURE);
      }
      printf("EVENT TYPE: %d\n",(event).type);
      switch((event).type)
      {
          case YAML_STREAM_START_EVENT:
              ++streamCounter;
              // printf("STREAM START: %s\n",event.data.scalar.value );
              break;
          case YAML_STREAM_END_EVENT:
              --streamCounter;
              // printf("STREAM END: %s\n",event.data.scalar.value );
              break;
          case YAML_DOCUMENT_START_EVENT:
              ++documentCounter;
              // printf("DOCUMENT START: %s\n",event.data.scalar.value );
              break;
          case YAML_DOCUMENT_END_EVENT:
              --documentCounter;
              // printf("DOCUMENT END: %s\n",event.data.scalar.value );
              break;
          case YAML_SEQUENCE_START_EVENT:
              ++sequenceCounter;
              // printf("SEQUENCE START: %s\n",event.data.mapping_start.anchor );
              break;
          case YAML_SEQUENCE_END_EVENT:
              --sequenceCounter;
              // printf("SEQUENCE END: %s\n",event.data.mapping_start.anchor );
              break;
          case YAML_MAPPING_START_EVENT:
              ++mappingCounter;
              // printf("MAPPING START: %s\n",event.data.mapping_start.anchor );
              break;
          case YAML_MAPPING_END_EVENT:
              --mappingCounter;
              // printf("MAPPING END: %s\n",event.data.mapping_start.anchor );
              break;
          case YAML_SCALAR_EVENT:
              printf("YAML VALUE: %s\n", event.data.scalar.value);
              parseDescriptionSection( &event, parser );
              break;
      }
      deleteEvent(&event);
      // if ( event->type != YAML_STREAM_END_EVENT )
      // {
      //     yaml_event_delete( &event );
      // }
  } while ((event).type != YAML_STREAM_END_TOKEN);
  yaml_event_delete( &event );
}

int main(int argc, char const *argv[])
{
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

  startParsingYamlFile(&parser);

  printf("Already parsed\n");
  // clean parser
  yaml_parser_delete( &parser );
  fclose( yaml_file );
  return SYSTEM_SUCCESS;
}
