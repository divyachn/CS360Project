#include <iostream>
#include "Camera.h"

using namespace std;

Camera::Camera(vec3 pos, vec3 dirn) {
  // constructor, specify the intial camera_coordinates, look_at vector, extremes of frustum, and near-far Z planes
  camera_coordinates = pos;
  look_at = dirn;
  // cout<<"Inside Camera constructor\n";
  // cout<<"Position: "<<camera_coordinates.x<<" "<<camera_coordinates.y<<" "<<camera_coordinates.z<<"\n";
  // cout<<"Direction: "<<look_at.x<<" "<<look_at.y<<" "<<look_at.z<<"\n";
  updateFrustum();
}

void Camera::updateFrustum(){
  // camera coordinates will determine the value of extremes - lrbt, and the Zplanes
  lrbt.x = camera_coordinates.x-50.0;  // left
  lrbt.y = camera_coordinates.x+50.0;  // right
  lrbt.z = camera_coordinates.y-50.0;  // bottom
  lrbt.w = camera_coordinates.y+50.0;  // top
  zplanes.x = 5.0;  // near distance (+ve) from the camera
  zplanes.y = 50.0; // far distance (+ve) from the camera
}

vec3 Camera::getCameraCoordinates() {
  return camera_coordinates;
}

void Camera::updateCameraCoordinates(vec3 pos) {
  camera_coordinates = pos;
  // updating the camera coordinates will also reflect in the value of extremes of the frustum
  updateFrustum();
}

vec3 Camera::getLookAtVector() {
  return look_at;
}

void Camera::updateLookAtVector(vec3 dirn) {
  look_at = dirn;
}

vec4 Camera::getlrbt() {
  return lrbt;
}

vec2 Camera::getZplanes() {
  return zplanes;
}
