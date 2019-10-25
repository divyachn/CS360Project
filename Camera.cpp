#include <Camera.h>

Camera::Camera(){
  // constructor
}

Camera::getCameraCoordinates(){
  return camera_coordinates;
}

Camera::getLookAtVector(){
  return look_at;
}
