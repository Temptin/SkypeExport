#!/bin/bash

if [ ! -d "release" ]; then
    mkdir release
fi

cd release

cmake -DCMAKE_BUILD_TYPE=Release ..
make

echo "Build complete! Check the ./release folder..."

