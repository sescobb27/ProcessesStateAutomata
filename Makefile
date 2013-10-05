CC 		= cc
CFLAGS 		= -g 
INCLUDES 		= -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBS 		= -lglib-2.0 
FINAL_LIBS= -lyaml 
OBJECT 		= parser
all: yaml_parser.c
	${CC} ${CFLAGS} ${INCLUDES} ${LIBS} ${CFLAGS} -o ${OBJECT} $< ${FINAL_LIBS}
