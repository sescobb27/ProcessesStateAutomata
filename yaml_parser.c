/*
USAR cc -o name yaml_parser.c -lyaml
LUEGO ./name file.yaml

cc yaml_parser.c -lyaml
gcc yaml_parser.c -lyaml
gcc -o name yaml_parser.c -lyaml
./name file.yaml
*/
#include "yaml.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "glib.h"
#include "string.h"

#define YAML_ERROR 0
#define YAML_SUCCESS 1
#define SYSTEM_SUCCESS 0
#define SYSTEM_ERROR 1
#define FILE_ERROR 0

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
    char *afabeto;
    char *estados;
    char *inicio;
    char *final;
    GSList* nodo_automata;
};

typedef struct automata_desc automata;
typedef struct automata_desc* pautomata;
typedef struct nodo_automata nodo;
typedef struct nodo_automata* pnodo;
typedef struct flujo_nodos flujo;
typedef struct flujo_nodos* pflujo;
typedef struct transicion_nodos transicion;
typedef struct transicion_nodos* ptransicion;

void
startParsingYamlFile(yaml_parser_t *parser)
{
  yaml_event_t event;
  do
  {
      // if( yaml_parser_parse(&parser, &event) == 0 )
      if( !yaml_parser_parse(parser, &event) )
      {
         printf("Parser Error %d\n", (*parser).error);
         exit(EXIT_FAILURE);
      }

      switch(event.type)
      {
          case YAML_SCALAR_EVENT:
            printf("YAML VALUE: %s \n", event.data.scalar.value);
            break;
      }
      if ( event.type != YAML_STREAM_END_EVENT )
      {
          yaml_event_delete( &event );
      }
  } while (event.type != YAML_STREAM_END_TOKEN);
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

  // init yaml parser
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
