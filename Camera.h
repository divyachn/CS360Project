#include <iostream>
#include <vector>
#include <glm/glm.hpp>

using glm::vec4;
using glm::vec3;
using glm::vec2;
using std::vector;

class Camera {

  private:
    vec3 camera_coordinates, look_at;
    vec4 lrbt;  // (x,y,z,w) = (left, right, bottom, top)
    vec2 zplanes; // (x,y) = (near, far) Z plane distance from the camera
    void updateFrustum();
  public:
    Camera(vec3, vec3);
  	vec3 getCameraCoordinates();
    vec3 getLookAtVector();
    vec4 getlrbt();
    vec2 getZplanes();
    void updateCameraCoordinates(vec3);
    void updateLookAtVector(vec3);
};
