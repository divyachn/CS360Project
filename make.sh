#!/bin/bash

echo "Compiling the code..."

echo "g++ -ggdb -std=c++11 -c -o shader_utils.o shader_utils.cpp"
g++ -ggdb -std=c++11 -c -o shader_utils.o shader_utils.cpp

echo "g++ -ggdb -std=c++11 -c -o texture.o texture.cpp"
g++ -ggdb -std=c++11 -c -o texture.o texture.cpp

echo "g++ -ggdb -std=c++11 -c -o maze.o MazeGenerator.cpp"
g++ -ggdb -std=c++11 -c -o maze.o MazeGenerator.cpp

echo "g++ -ggdb -std=c++11 -c -o camera.o Camera.cpp"
g++ -ggdb -std=c++11 -c -o camera.o Camera.cpp

echo "g++ -ggdb -std=c++11 main.cpp shader_utils.o texture.o camera.o maze.o -lglut -lGLEW -lGL -lGLU -lm -lalut -lopenal -o game"
g++ -ggdb -std=c++11 main.cpp shader_utils.o texture.o camera.o maze.o -lglut -lGLEW -lGL -lGLU -lm -lalut -lopenal -o game
