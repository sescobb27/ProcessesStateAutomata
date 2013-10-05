GCC = g++
INCLUDES = -I/usr/local/include -L/usr/local/lib -lyaml-cpp
CFLAGS = -Wall -g
OBJECT = parser
all: yaml_parser.cpp
	${GCC} ${CFLAGS} $< ${INCLUDES} ${LIBS} -o ${OBJECT}
