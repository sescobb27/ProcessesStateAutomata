#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include "automata.h"

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

int yamlParser(yaml_event_t *event, yaml_parser_t *parser, char *where ){
  if( !yaml_parser_parse(parser, event) ) {
     printErrorMsg( where, (*parser).problem );
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
void next(yaml_event_t *event, yaml_parser_t *parser, char *where){
  if( !deleteEvent( event ) )
      exit(EXIT_FAILURE);
  yamlParser(event, parser, where);
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
  fprintf(stdout, INFO_FORMAT, automata_name, getpid());
}

void printAcceptMsg( char *automata_name, char *msg ) {
  fprintf(stdout, ACCEPT_FORMAT, automata_name, msg );
}

void printRejectMsg( char *automata_name, char *msg, int pos ) {
  fprintf(stdout, REJECT_FORMAT, automata_name, msg, pos );
}

void printErrorMsg( char *where, const char *cause ) {
  fprintf(stderr, ERROR_FORMAT, where, cause );
}

void* readingThreadController(void *args) {
  p_type_automata pautomata = (p_type_automata) args;
  char *buffer = (char*) malloc( sizeof(char) * MAX_WORD_LENGTH );
  memset( buffer, '\0', MAX_WORD_LENGTH);
  char *data = (char*) malloc ( sizeof(char) * MAX_WORD_LENGTH );
  memset( data, '\0', MAX_WORD_LENGTH);
  char *recog;
  char *where = (char*) malloc( sizeof(char) * MAX_WORD_LENGTH );
  memset( where, '\0', MAX_WORD_LENGTH);
  sprintf( where, "\"Pid: %d, automata: %s, on function readingThreadController\"", getpid(), pautomata->nombre );
  int codterm;
  while ( 1 ) {
    while ( read( pautomata->pipe_to_father[0], buffer, MAX_WORD_LENGTH ) > 0 ) {
      yaml_parser_t parser;
      yaml_event_t event;
      if ( !yaml_parser_initialize( &parser ) ) {
          printErrorMsg(where, "Unable to initialize yaml parser");
          exit(EXIT_FAILURE);
      }
      yaml_parser_set_input_string( &parser, buffer, strlen( buffer + 1 ) );
      if ( yamlParser( &event, &parser, where ) ){
        while (event.type != YAML_STREAM_END_EVENT) {
          if (event.type != YAML_SCALAR_EVENT ) {
              next(&event, &parser, where);
              continue;
          } else {
            strcpy( data, event.data.scalar.value );
            if ( strcmp( data, diccionario[CODTERM] ) == 0) {
              next(&event,&parser, where);
              if ( event.type == YAML_SCALAR_EVENT ) {
                codterm = atoi( event.data.scalar.value );
                next(&event, &parser, where);
              }
            } else if ( strcmp( data, diccionario[RECOG] ) == 0 ) {
                next(&event,&parser, where);
                if ( event.type == YAML_SCALAR_EVENT ) {
                    recog = (char*) malloc( sizeof(char) * strlen( event.data.scalar.value ) + 1);
                    strcpy( recog, event.data.scalar.value );
                    if ( codterm == 0 ) {
                      printAcceptMsg( pautomata->nombre, recog );
                      break;
                    }
                    next(&event,&parser, where);
                }
           } else if ( strcmp( data, diccionario[REST] ) == 0 && codterm == 1 ){
                next(&event, &parser, where);
                if ( event.type == YAML_SCALAR_EVENT ) {
                    strcpy( data, recog );
                    strcat( data, event.data.scalar.value );
                    printRejectMsg( pautomata->nombre, data, strlen(recog) + 1);
                    break;
                }
            } else { break ;}
          }
        }
        yaml_parser_delete( &parser );
      }
    }
  }
}

void sendCommand(char *command, char *msg, p_type_automata pautomata) {
  p_type_automata aux = pautomata;
  char *_msg = (char*) malloc( sizeof(msg) * MAX_WORD_LENGTH );
  memset(_msg, '\0', MAX_WORD_LENGTH);
  if ( strcmp(command, diccionario[INFO] ) == 0) {
    for (; aux; aux = aux->next) {
      if ( strlen(msg) == 0){
        nodes_printer(aux, _msg);
      } else if ( strcmp(msg, aux->nombre) == 0) {
        nodes_printer(aux, _msg);
        break;
      }
    }
  } else if ( strcmp(command, diccionario[SEND] ) == 0) {
    p_type_nodo _n_aux;
    yamlStringFormater( _msg, "", msg );
    for (; aux; aux = aux->next) {
      for ( _n_aux = aux->primer_nodo; _n_aux ; _n_aux = _n_aux->next ) {
        if ( strcmp( _n_aux->id, aux->estadoinicial ) == 0 ) {
          write( _n_aux->fd[1], _msg, strlen(_msg) );
          break;
        }
      }
    }
  } else if ( strcmp(command, diccionario[STOP]) == 0) {
    // 0 = All processes in the same process group as the sender.
    kill( 0, SIGKILL);
  }
}

void startListeningUserInput( p_type_automata pautomata) {
  yaml_parser_t parser;
  yaml_event_t event;
  char *data, *command, *msg;
  data = (char*) malloc(sizeof(char) * MAX_WORD_LENGTH);
  memset(data, '\0', MAX_WORD_LENGTH);
  command = (char*) malloc(sizeof(char) * MAX_WORD_LENGTH);
  memset(command, '\0', MAX_WORD_LENGTH);
  msg = (char*) malloc(sizeof(char) * MAX_WORD_LENGTH);
  memset( msg, '\0', MAX_WORD_LENGTH );
  char *where = "on startListeningUserInput";
  while( 1 ) {
    if ( !yaml_parser_initialize( &parser ) ){
      printErrorMsg(where, "Unable to initialize yaml parser");
      exit(EXIT_FAILURE);
    }
    fgets(msg, MAX_WORD_LENGTH, stdin);
    size_t size = strlen(msg) - 1;
    if (msg[size] == '\n'){
      msg[size] = '\0';
    }
    yaml_parser_set_input_string( &parser, msg, size );
    if (yamlParser( &event,&parser,  where )) {
      while (event.type != YAML_STREAM_END_EVENT) {
        if (event.type != YAML_SCALAR_EVENT ) {
          next(&event, &parser, where);
          continue;
        } else {
          strcpy( data, event.data.scalar.value );
          if ( strcmp( data, diccionario[CMD] ) == 0 ) {
            next(&event,&parser, where);
            if ( event.type == YAML_SCALAR_EVENT ) {
              strcpy( data, event.data.scalar.value );
              command = getCommand(data );
              if ( !command ) {
                printErrorMsg(where, "Command Not Found");
                break;
              }
              next(&event,&parser, where);
           }
         } else if ( strcmp( data, diccionario[MSG] ) == 0 ){
            next(&event, &parser, where);
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

void processController(p_type_nodo *pnodo, char* nombre_automata, char **estados_finales, int size_finales/*, int *fd*/) {
    char *buffer = (char *) malloc( sizeof(char) * MAX_WORD_LENGTH);
    memset(buffer, '\0', MAX_WORD_LENGTH);
    char *recog;
    char *data = (char*) malloc( sizeof( char ) * MAX_WORD_LENGTH );
    char *_msg = (char*) malloc( sizeof( char ) * MAX_WORD_LENGTH );
    memset(_msg, '\0', MAX_WORD_LENGTH);
    int send = 0, check = 0;
    char *where = "on processController";
    while(1)
    {
      while (read((*pnodo)->fd[0], buffer, MAX_WORD_LENGTH) > 0) {
        _msg[0] = '\0';
        yaml_parser_t parser;
        yaml_event_t event;
        send = 0;
        check = 0;
        if ( !yaml_parser_initialize( &parser ) ){
            printErrorMsg(where, "Unable to initialize yaml parser");
            exit(EXIT_FAILURE);
        }
        yaml_parser_set_input_string( &parser, buffer, strlen( buffer + 1 ) );
        if ( yamlParser( &event, &parser, where ) ){
            while (event.type != YAML_STREAM_END_EVENT) {
                if (event.type != YAML_SCALAR_EVENT ) {
                    next(&event, &parser, where);
                    continue;
                } else {
                  strcpy( data, event.data.scalar.value );
                  if ( strcmp( data, diccionario[RECOG] ) == 0 ) {
                      next(&event,&parser, where);
                      if ( event.type == YAML_SCALAR_EVENT ) {
                          recog = (char*) malloc( sizeof(char) * strlen( event.data.scalar.value ) + 1);
                          strcpy( recog, event.data.scalar.value );
                          next(&event,&parser, where);
                     }
                 } else if ( strcmp( data, diccionario[REST] ) == 0 ){
                      next(&event, &parser, where);
                      if ( event.type == YAML_SCALAR_EVENT ) {
                          strcpy( data, event.data.scalar.value );
                      }
                  } else { break ;}
                }
            }
        }
        p_type_transicion aux = (*pnodo)->primer_transicion;
        if ( strlen( data ) > 0) {
          for(; aux; aux = aux->next) {
              if ( memcmp(aux->entrada, data, strlen(aux->entrada) ) == 0 ) {
                  strncat(recog, data, strlen(aux->entrada));
                  data+= strlen(aux->entrada);
                  if (  strlen(data) == 0 )
                    strcpy(data, "\"\"");
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

int createProcessPerNode(p_type_nodo pnodo, char* nombre_automata, int nodo, char **estados_finales, int size_finales) {
  p_type_nodo aux = NULL;
  char *where = "on createProcessPerNode";
  for( aux = pnodo; aux; aux = aux->next) {
    if (pipe(fd_padre[nodo]) == -1) {
       printErrorMsg( where , "creating pipe for nodes" );
       exit(EXIT_FAILURE);
     }
    aux->fd = fd_padre[nodo];
    ++nodo;
  }
  pid_t id;
  for( aux = pnodo; aux; aux = aux->next) {
    if ( (id = fork()) == 0 ) {
      // dentro del proceso hijo
      processController( &aux, nombre_automata, estados_finales, size_finales/*, fd_padre[nodo] */);
      break;
    }else{
      // dentro del padre
      aux->pid = id;
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
    pthread_create(&(aux->hilo_lectura), NULL, readingThreadController, (void *) aux);
    nodo = createProcessPerNode(aux->primer_nodo, aux->nombre, nodo, aux->final, aux->sizeFinal);
  }
  startListeningUserInput( pautomata );
}

void parseTransitions(yaml_parser_t *parser, yaml_event_t *event, p_type_nodo *pnodo){
  char *where = "on parseTransitions";
  next(event, parser, where);
  char *data = (char *)malloc(sizeof(char) * MAX_WORD_LENGTH);
  strcpy(data, event->data.scalar.value);
  if ( strcmp( data, diccionario[TRANS]) != 0 )
      exit(EXIT_FAILURE);
  next(event, parser, where);
  (*pnodo)->transiciones = NULL;
  if (event->type != YAML_SEQUENCE_START_EVENT) {
    next(event, parser, where);
    return;
  }
  next(event, parser, where);
  while( event->type == YAML_MAPPING_START_EVENT ) {
    next(event, parser, where);
    strcpy(data, event->data.scalar.value);
    if ( strcmp( data, diccionario[IN]) == 0 ) {
      p_type_transicion ptransicion = ( p_type_transicion) malloc( sizeof( transicion ) );
      ptransicion->entrada = malloc( sizeof(char) * MAX_WORD_LENGTH );
      ptransicion->sig_estado = malloc( sizeof(char) * MAX_WORD_LENGTH );
      // pedir proximo evento y asignarselo a la entrada
      next(event, parser, where);
      strcpy(data, event->data.scalar.value);
      strcpy( ptransicion->entrada, data );
      // el proximo tag es next que nos indica el estado para donde vamos
      next(event, parser, where);
      // pedir proximo evento y asignarselo a el siguiendo estado
      next(event, parser, where);
      strcpy(data, event->data.scalar.value);
      strcpy( ptransicion->sig_estado, data );
      if ((*pnodo)->transiciones == NULL){
        (*pnodo)->primer_transicion = ptransicion;
        (*pnodo)->transiciones = ptransicion;
      } else {
        (*pnodo)->transiciones->next = ptransicion;
        (*pnodo)->transiciones = ptransicion;
        ptransicion = ptransicion->next;
      }
      // volver a pedir el proximo evento para poder ver si es o no otra transicion
      next(event, parser, where);
      if (event->type == YAML_MAPPING_END_EVENT) {
        /* code */
        next(event, parser, where);
      }
    }
  }
}

void parseNodesSection(yaml_parser_t *parser, yaml_event_t *event, p_type_automata *pautomata) {
  char *where = "on parseNodesSection";
  next(event, parser, where);
  char *data = (char *)malloc(sizeof(char) * MAX_WORD_LENGTH);
  int size = 0;
  strcpy(data, event->data.scalar.value);
  if ( strcmp(data, diccionario[DELTA]) == 0 ) {
        next(event, parser, where);
        if (event->type == YAML_SEQUENCE_START_EVENT){
              next(event, parser, where);
        }else{
              exit(EXIT_FAILURE);
        }
      p_type_nodo pnodo = NULL;
      (*pautomata)->nodos = NULL;
      while (event->type == YAML_MAPPING_START_EVENT){
            next(event, parser, where);
            strcpy(data, event->data.scalar.value);
            if ( strcmp( data, diccionario[NODE]) == 0){
                  next(event, parser, where);
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
                        next(event, parser, where);
                        if( event->type == YAML_MAPPING_END_EVENT){
                          next( event, parser, where);
                        }
                  }
                  ++size;
            }
      }
  }
}

void parseSequenceSection(yaml_event_t *event, yaml_parser_t *parser, int kind, p_type_automata *pautomata) {
    char *data = (char*) malloc(sizeof(char) * MAX_WORD_LENGTH);
    char *where = "on parseSequenceSection";
    switch(kind)
    {
      case ALPHA:
      { //scope for ALPHA case http://stackoverflow.com/questions/92396/why-cant-variables-be-declared-in-a-switch-statement
          int pos_alfab = 0;
          next(event, parser, where);
          while(event->type == YAML_SCALAR_EVENT) {
              strcpy(data, event->data.scalar.value);
              (*pautomata)->alfabeto[pos_alfab] = (char *) malloc(event->data.scalar.length);
              strcpy((*pautomata)->alfabeto[pos_alfab], data);
              ++pos_alfab;
              next(event, parser, where);
          }
          if( !pos_alfab)
              exit(EXIT_FAILURE);
          (*pautomata)->sizeAlfabeto = pos_alfab;
          break;
      }
      case STATES:
      { //scope for STATES case
          int pos_alfab = 0;
          next(event, parser, where);
          while(event->type == YAML_SCALAR_EVENT) {
              strcpy(data, event->data.scalar.value);
              (*pautomata)->estados[pos_alfab] = (char *) malloc(event->data.scalar.length);
              strcpy((*pautomata)->estados[pos_alfab], data);
              ++pos_alfab;
              next(event, parser, where);
          }
          if( !pos_alfab)
              exit(EXIT_FAILURE);
          (*pautomata)->sizeEstados = pos_alfab;
          break;
      }
      case FINAL:
      {
        int pos_alfab = 0;
        next(event, parser, where);
        while(event->type == YAML_SCALAR_EVENT) {
            strcpy(data, event->data.scalar.value);
            (*pautomata)->final[pos_alfab] = (char *) malloc(event->data.scalar.length);
            strcpy((*pautomata)->final[pos_alfab], data);
            ++pos_alfab;
            next(event, parser, where);
        }
        if( !pos_alfab)
            exit(EXIT_FAILURE);
        (*pautomata)->sizeFinal = pos_alfab;
        break;
      }
    }
}

p_type_automata automataParser(yaml_event_t *event, yaml_parser_t *parser) {
  char *where = "on automataParser";
  char *data = (char *)malloc(sizeof(char) * MAX_WORD_LENGTH);
  strcpy(data, event->data.scalar.value);
  p_type_automata pautomata = (p_type_automata)  malloc( sizeof( automata ) );
  initStringArray(&pautomata);
  // si el dato es = automata pido el siguiente evento que debe de ser el nombre del automata
  if (strcmp( data, diccionario[AUTOMATA] ) == 0) {
        next(event, parser, where);
        pautomata->nombre = (char*) malloc(event->data.scalar.length);
        strcpy(pautomata->nombre, event->data.scalar.value);
  }
  // el siguiente evento debe de ser la descripcion
  next(event, parser, where);
  strcpy(data, event->data.scalar.value);
  if (strcmp( data, diccionario[DESCRIPTION] ) == 0) {
        next(event, parser, where);
        pautomata->descripcion = (char*) malloc(event->data.scalar.length);
        strcpy(pautomata->descripcion, event->data.scalar.value);
        // fprintf(stdout, "\t%s\n", pautomata->descripcion);
  }
  // el siguiente evento debe de ser el alfabeto, el cual contiene una secuencia donde
  // los caracteres estan encerrados en forma de arreglo
  next(event, parser, where);
  strcpy(data, event->data.scalar.value);
  if (strcmp( data, diccionario[ALPHA] ) == 0) {
        // el siguiente evento debe de ser una secuencia de letras que componen al alfabeto
        next(event, parser, where);
        switch (event->type)
        {
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
  next(event, parser, where);
  strcpy(data, event->data.scalar.value);
  if (strcmp( data, diccionario[STATES] ) == 0) {
        next(event, parser, where);
        switch (event->type)
        {
          case YAML_SEQUENCE_START_EVENT:
              parseSequenceSection(event, parser, STATES, &pautomata);
              if (event->type != YAML_SEQUENCE_END_EVENT)
                exit(EXIT_FAILURE);
              break;
        }
  }
  next(event, parser, where);
  strcpy(data, event->data.scalar.value);
  if (strcmp( data, diccionario[START] ) == 0) {
    next(event, parser, where);
    pautomata->estadoinicial = (char *) malloc(sizeof(char) * MAX_WORD_LENGTH);
    strcpy(pautomata->estadoinicial, event->data.scalar.value);
  }
  next(event, parser, where);
  strcpy(data, event->data.scalar.value);
  if (strcmp( data, diccionario[FINAL] ) == 0) {
        next(event, parser, where);
        switch (event->type)
        {
          case YAML_SEQUENCE_START_EVENT:
              parseSequenceSection(event, parser, FINAL, &pautomata);
              if (event->type != YAML_SEQUENCE_END_EVENT)
                exit(EXIT_FAILURE);
              break;
        }
  }
  pautomata->pipe_to_father = (int*) malloc( sizeof(int) * 2);
   if ( pipe(pautomata->pipe_to_father) == -1 ) {
    printErrorMsg( where, "Error creating pipe_to_father" );
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
      yamlParser( &event, parser, "on startParsingYamlFile" );
      switch((event).type)
      {
          case YAML_SCALAR_EVENT:
              if (pautomata == NULL){
                  primer_pautomata = automataParser( &event, parser );
                  pautomata = primer_pautomata;
              }else {
                  pautomata->next = automataParser( &event, parser );
                  pautomata = pautomata->next;
              }
              break;
      }
      deleteEvent(&event);
  } while ((event).type != YAML_STREAM_END_TOKEN);
  yaml_event_delete( &event );
  return primer_pautomata;
}

int main(int argc, char const *argv[]){
  setpgid( getpid(), getpid() );
  if ( argc < 2 ) {
    printErrorMsg("Main", "There is no file to parse, try again");
    return SYSTEM_ERROR;
  }
  FILE *yaml_file = fopen( argv[1], "r" );
  if ( yaml_file == FILE_ERROR ) {
    printErrorMsg("Main", "Couldn't open file");
    return SYSTEM_ERROR;
  }
  yaml_parser_t parser;

  if ( !yaml_parser_initialize( &parser ) ) {
    printErrorMsg("Main", "Unable to initialize yaml parser");
    return SYSTEM_ERROR;
  }
  yaml_parser_set_input_file( &parser, yaml_file );

  p_type_automata pautomata = NULL;
  pautomata = startParsingYamlFile(&parser);
  yaml_parser_delete( &parser );
  fclose( yaml_file );

  crearHijos(pautomata);
  return SYSTEM_SUCCESS;
}
