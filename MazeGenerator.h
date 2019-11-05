#include <utility>
#include <map>
#include <stdlib.h>
#include <iostream>

// #define LEFT 0
// #define RIGHT 1
// #define UP 2
// #define DOWN 3
//
// #define GRID_X 150
// #define GRID_Z 150

// int x_move[] = {-1, 1, 0, -1, 1, 0, -1, 1};
// int z_move[] = {0, 0, 1, 1, 1, -1, -1, -1};

class Maze{
private:
  int gridx, gridz;
  int x_move[4], z_move[4];
  int maze[150][150];
public:
  Maze(int , int );
  void generateMaze();
  bool isValid(int , int);
};
