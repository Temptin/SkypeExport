#!/bin/bash

echo "Building SkypeExport (Intel 32/64bit Universal Binary for Mac OS X)..."
gcc -arch i386 -arch x86_64 -O3 -c "../libs/sqlite3/sqlite3.c" -o sqlite3.o
g++ -arch i386 -arch x86_64 -O3 -c "../model/skypeparser_core.cpp" -o skypeparser_core.o \
	-I /usr/local/include 
g++ -arch i386 -arch x86_64 -O3 -c "../model/skypeparser_parsing.cpp" -o skypeparser_parsing.o \
	-I /usr/local/include 
g++ -arch i386 -arch x86_64 -O3 -c "../main.cpp" -o main.o \
	-I /usr/local/include 
g++ -arch i386 -arch x86_64 sqlite3.o skypeparser_core.o skypeparser_parsing.o main.o -o SkypeExport \
	-L /usr/local/lib \
	-lboost_filesystem \
	-lboost_program_options \
	-lboost_regex \
	-lboost_system
echo "Build complete!"