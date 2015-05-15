#!/bin/bash

echo "Building SkypeExport (Intel 32/64bit Universal Binary for Mac OS X)..."
mkdir -p "buildcache"

if [[ -f "buildcache/sqlite3.o" && "buildcache/sqlite3.o" -nt "../libs/sqlite3/sqlite3.c" && "buildcache/sqlite3.o" -nt "../libs/sqlite3/sqlite3.h" ]]; then
	echo " - Using Build Cache: sqlite3.o"
else
	echo " - Building (New/Modified): sqlite3.o"
	gcc -arch i386 -arch x86_64 -O3 -c "../libs/sqlite3/sqlite3.c" -o "buildcache/sqlite3.o"
fi

if [[ -f "buildcache/skypeparser_core.o" && "buildcache/skypeparser_core.o" -nt "../model/skypeparser_core.cpp" && "buildcache/skypeparser_core.o" -nt "../model/skypeparser.h" && "buildcache/skypeparser_core.o" -nt "../libs/sqlite3/sqlite3.h" ]]; then
	echo " - Using Build Cache: skypeparser_core.o"
else
	echo " - Building (New/Modified): skypeparser_core.o"
	g++ -arch i386 -arch x86_64 -O3 -c "../model/skypeparser_core.cpp" -o "buildcache/skypeparser_core.o" \
		-I /usr/local/include
fi

if [[ -f "buildcache/skypeparser_parsing.o" && "buildcache/skypeparser_parsing.o" -nt "../model/skypeparser_parsing.cpp" && "buildcache/skypeparser_parsing.o" -nt "../model/skypeparser.h" && "buildcache/skypeparser_parsing.o" -nt "../libs/sqlite3/sqlite3.h" ]]; then
	echo " - Using Build Cache: skypeparser_parsing.o"
else
	echo " - Building (New/Modified): skypeparser_parsing.o"
	g++ -arch i386 -arch x86_64 -O3 -c "../model/skypeparser_parsing.cpp" -o "buildcache/skypeparser_parsing.o" \
		-I /usr/local/include
fi

if [[ -f "buildcache/main.o" && "buildcache/main.o" -nt "../main.cpp" && "buildcache/main.o" -nt "../model/skypeparser.h" && "buildcache/main.o" -nt "../libs/sqlite3/sqlite3.h" ]]; then
	echo " - Using Build Cache: skypeparser_core.o"
else
	echo " - Building (New/Modified): main.o"
	g++ -arch i386 -arch x86_64 -O3 -c "../main.cpp" -o "buildcache/main.o" \
		-I /usr/local/include
fi

echo " - Linking: SkypeParser"
mkdir -p "release"
g++ -arch i386 -arch x86_64 "buildcache/sqlite3.o" "buildcache/skypeparser_core.o" "buildcache/skypeparser_parsing.o" "buildcache/main.o" -o "release/SkypeExport" \
	-L /usr/local/lib \
	-lboost_filesystem \
	-lboost_program_options \
	-lboost_regex \
	-lboost_system

echo "Build complete! Check the ./release folder..."
