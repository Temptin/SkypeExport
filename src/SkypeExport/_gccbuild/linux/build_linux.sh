#!/bin/bash

echo "Building SkypeExport for Linux..."

if [ ! -d "release" ]; then
    mkdir release
fi

cd release

cmake -DCMAKE_BUILD_TYPE=Release ..
[ $? -ne 0 ] && { echo "Compilation failed..."; exit 1; }
make
[ $? -ne 0 ] && { echo "Compilation failed..."; exit 1; }

echo "Build complete! Check the ./release folder..."

