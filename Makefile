CC 		= cc
CFLAGS 		= -g -pthread
LIBS = -lyaml
OBJECT 		= sisctrl
HEADER = src/automata.h
all: src/yaml_parser.c
	${CC} ${HEADER}
	${CC} ${CFLAGS} -o ${OBJECT} $< ${LIBS}
