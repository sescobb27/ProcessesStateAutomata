# Process Automata

## Used libraries

Unix library for creating pipes, processes and group processes:

```c
    #include <unistd.h>
```

Thread library for handling POSIX thread programming:
```c
    #include <pthread.h>
```

Yaml library for parsing yaml configuration file (See install for more information):

```c
    #include "yaml.h"
```

Standard Input/output library which handle standard out, in and error streams for get getting users input, yaml configuration file
and printing info/error messages:

```c
    #include <stdio.h>
```

C library for asigning memory to data structures, pointers, array of pointers and exit program function:
```c
    #include <stdlib.h>
```

```c
    #include <sys/types.h>
```

Signal Handling for killing a process group:
```c
    #include <signal.h>
```