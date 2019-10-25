#include <iostream>
#include <vector>
#include <glm\glm.hpp>

using glm::vec3;
using glm::vec2;
using std::vector;

class Camera {

  private:
    vec3 camera_coordinates;
    vec3 look_at;
  public:
  	vec3 getCameraCoordinates();
    vec3 getLookAtVector();
    void updateCamera
};
