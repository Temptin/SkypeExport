#1/bin/bash

g++ -arch i386 -arch x86_64 -O3 -c "../file2header/file2header/file2header.cpp" -o file2header.o

g++ -arch i386 -arch x86_64 file2header.o -o file2header

rm -f file2header.o
