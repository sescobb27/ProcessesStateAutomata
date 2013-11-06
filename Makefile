CC 		= cc
CFLAGS 		= -g -pthread
LIBS = -lyaml
OBJECT 		= sisctrl
all: yaml_parser.c
	${CC} ${CFLAGS} -o ${OBJECT} $< ${LIBS}
