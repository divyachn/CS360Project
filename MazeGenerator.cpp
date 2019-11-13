#include "MazeGenerator.h"

using namespace std;

Maze::Maze(int dimx, int dimz){
  gridx = dimx;
  gridz = dimz;
  // 0:LEFT 1:RIGHT 2:UP 3:DOWN
  x_move[0] = -1; x_move[1] = 1; x_move[2] = 0; x_move[3] = 0;
  z_move[0] = 0; z_move[1] = 0; z_move[2] = 1; z_move[3] = -1;
  for(int i=0; i<150; i++)
    for(int j=0; j<150; j++)
      maze.arr[i][j]='-';
}

bool Maze::isValid(int xcoord, int zcoord){
  if(xcoord<0 || xcoord>gridx || zcoord<0 || zcoord>gridz) return false;
  else if(maze.arr[zcoord][xcoord]!='-') return false; // this cell has already been visited
  else return true;
}

struct Grid Maze::generateMaze(){
  // figure out a path to the end of the maze
  int start_x = gridx/2;
  int start_z = 0;
  int i,j;
  for(i=99;i>=75;i--) maze.arr[i][start_x]='U';
  for(j=start_x;j>=25;j--) maze.arr[75][j]='L';
  for(i=74;i>=50;i--) maze.arr[i][25]='U';
  for(j=24;j>=0;j--) maze.arr[50][j]='L';
  for(i=50;i>=0;i--) maze.arr[i][0]='U';
  cout<<"maze generated successfully!\n";
  return maze;
}
