# Process Automata

## Used libraries

Unix library for creating pipes, processes and group processes:

```c++
    #include <unistd.h>
```

Thread library for handling POSIX thread programming (functions and headers):

```c++
    #include <pthread.h>
```

Yaml library for parsing yaml configuration file (See install for more information):

```c++
	#include "yaml-cpp/yaml.h"
```

Standard Input/output library which handle standard out, in and error streams for get getting users input, yaml configuration file
and printing info/error messages:

```c++
    #include <stdio.h>
```

Library for asigning memory to data structures, pointers, array of pointers and exit program function:

```c++
    #include <stdlib.h>
```

Signal Handling for killing a process group:

```c++
    #include <signal.h>
```
Input/output stream class to operate on files.
Objects of this class maintain a filebuf object as their internal stream buffer, which performs input/output operations on the file they are associated with (if any).

```c++
	#include <fstream>
```

Library which has pid_t and pthread_t data structures for handling proccesses and pthreads:

```c++
	#include <sys/types.h>
``` 	

Header that defines the standard input/output stream objects

```c++
	#include <iostream>
```

This header file defines several functions to manipulate C strings and arrays.

```c++
	#include <string.h>
```
