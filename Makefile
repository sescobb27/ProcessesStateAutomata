CC 		= cc
CFLAGS 		= -g -pthread
INCLUDES 		= `pkg-config --libs --cflags glib-2.0`
LIBS = -lyaml
OBJECT 		= sisctrl
all: yaml_parser.c
	${CC} ${CFLAGS} -o ${OBJECT} $< ${INCLUDES} ${LIBS}
