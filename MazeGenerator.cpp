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
      maze.arr[i][j]=0;
}

bool Maze::isValid(int xcoord, int zcoord){
  if(xcoord<0 || xcoord>gridx || zcoord<0 || zcoord>gridz) return false;
  else if(maze.arr[zcoord][xcoord]!=0) return false; // this cell has already been visited
  else return true;
}

struct Grid Maze::generateMaze(){
  int start_x = rand()%gridx;
  int start_z = 0;

  while(start_z != gridz-1){
    maze.arr[start_z][start_x] = 1;
    int t = rand()%4; // t is a random number generated in the range [0...3]
    int newx = start_x + x_move[t];
    int newz = start_z + z_move[t];
    if(isValid(newx, newz)){
      // the new generated point is valid, it can be added to the generated path
      start_z=newz;
      start_x=newx;
    } else{
      while(start_z<gridz-1 && maze.arr[start_z][start_x]!=0) start_z++;
    }
  }
  maze.arr[start_z][start_x]=1;
  return maze;
}
