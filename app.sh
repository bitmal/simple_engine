#!/usr/bin/sh

rm -rf ./build
mkdir ./build
pushd ./build

gcc -std=c11 -g -o simple_engine ../src/engine/main.c -lm -lGL -lSDL3 -lGLEW 

popd
