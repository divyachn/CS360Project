#include <utility>
#include <map>
#include <stdlib.h>
#include <iostream>

struct Grid {
  char arr[150][150];
};

class Maze{
private:
  int gridx, gridz;
  int x_move[4], z_move[4];
  struct Grid maze;
public:
  Maze(int , int );
  struct Grid generateMaze();
  bool isValid(int , int);
};
