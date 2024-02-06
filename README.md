Commands to replace your current memalloc with this:
1. $ gcc -o memalloc.so -fPIC -shared memalloc.c
2. $ export LD_PRELOAD=/$PWD/memalloc.so
