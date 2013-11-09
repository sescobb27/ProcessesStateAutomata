CC 		= cc
CFLAGS 		= -g -pthread
LIBS = -lyaml
OBJECT 		= sisctrl
HEADER = automata.h
all: yaml_parser.c
	${CC} ${HEADER}
	${CC} ${CFLAGS} -o ${OBJECT} $< ${LIBS}
