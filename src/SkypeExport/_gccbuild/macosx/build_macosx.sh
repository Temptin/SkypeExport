#!/bin/bash

# Build configuration. Useful if you've got Boost in a different path,
# or if you only want to build a certain architecture (in that case,
# simply remove the -arch statement that you don't want).
opt_arch=-arch i386 -arch x86_64
opt_localpath=/usr/local

echo "Building SkypeExport (Intel 32/64bit Universal Binary for Mac OS X)..."
mkdir -p "buildcache"

if [[ -f "buildcache/sqlite3.o" && "buildcache/sqlite3.o" -nt "../../libs/sqlite3/sqlite3.c" && "buildcache/sqlite3.o" -nt "../../libs/sqlite3/sqlite3.h" ]]; then
	echo " - Using Build Cache: sqlite3.o"
else
	echo " - Building (New/Modified): sqlite3.o"
	gcc $opt_arch -O3 -c "../../libs/sqlite3/sqlite3.c" -o "buildcache/sqlite3.o"
	[ $? -ne 0 ] && { echo "Compilation failed..."; exit 1; }
fi

if [[ -f "buildcache/skypeparser_core.o" && "buildcache/skypeparser_core.o" -nt "../../model/skypeparser_core.cpp" && "buildcache/skypeparser_core.o" -nt "../../model/skypeparser.h" && "buildcache/skypeparser_core.o" -nt "../../libs/sqlite3/sqlite3.h" && "buildcache/skypeparser_core.o" -nt "../../resources/css_and_images/style_compact_data_css.h" ]]; then
	echo " - Using Build Cache: skypeparser_core.o"
else
	echo " - Building (New/Modified): skypeparser_core.o"
	g++ $opt_arch -O3 -c "../../model/skypeparser_core.cpp" -o "buildcache/skypeparser_core.o" \
		-I $opt_localpath/include
	[ $? -ne 0 ] && { echo "Compilation failed..."; exit 1; }
fi

if [[ -f "buildcache/skypeparser_parsing.o" && "buildcache/skypeparser_parsing.o" -nt "../../model/skypeparser_parsing.cpp" && "buildcache/skypeparser_parsing.o" -nt "../../model/skypeparser.h" && "buildcache/skypeparser_parsing.o" -nt "../../libs/sqlite3/sqlite3.h" && "buildcache/skypeparser_parsing.o" -nt "../../resources/css_and_images/style_compact_data_css.h" ]]; then
	echo " - Using Build Cache: skypeparser_parsing.o"
else
	echo " - Building (New/Modified): skypeparser_parsing.o"
	g++ $opt_arch -O3 -c "../../model/skypeparser_parsing.cpp" -o "buildcache/skypeparser_parsing.o" \
		-I $opt_localpath/include
	[ $? -ne 0 ] && { echo "Compilation failed..."; exit 1; }
fi

if [[ -f "buildcache/main.o" && "buildcache/main.o" -nt "../../main.cpp" && "buildcache/main.o" -nt "../../model/skypeparser.h" && "buildcache/main.o" -nt "../../libs/sqlite3/sqlite3.h" && "buildcache/main.o" -nt "../../resources/css_and_images/style_compact_data_css.h" ]]; then
	echo " - Using Build Cache: main.o"
else
	echo " - Building (New/Modified): main.o"
	g++ $opt_arch -O3 -c "../../main.cpp" -o "buildcache/main.o" \
		-I $opt_localpath/include
	[ $? -ne 0 ] && { echo "Compilation failed..."; exit 1; }
fi

echo " - Linking: SkypeParser"
mkdir -p "release"
g++ $opt_arch "buildcache/sqlite3.o" "buildcache/skypeparser_core.o" "buildcache/skypeparser_parsing.o" "buildcache/main.o" -o "release/SkypeExport" \
	-L $opt_localpath/lib \
	-lboost_filesystem \
	-lboost_program_options \
	-lboost_regex \
	-lboost_system
[ $? -ne 0 ] && { echo "Compilation failed..."; exit 1; }

echo "Build complete! Check the ./release folder..."
