GCC = g++
INCLUDES = -lyaml-cpp -pthread
CFLAGS = -Wall -g
OBJECT = sisctrl
HEADER = automata.h
all: yaml_parser.cpp
	${GCC} ${HEADER}
	${GCC} ${CFLAGS} $< ${INCLUDES} -o ${OBJECT}
