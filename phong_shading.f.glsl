/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Martin Kraus, Sylvain Beucler
 */

varying vec4 position;  // position of the vertex (and fragment) in world space
varying vec3 varyingNormalDirection;  // surface normal vector in world space
varying vec4 color;
varying vec2 tex_coord;

uniform sampler2D tex;
uniform mat4 m, v, p;
uniform mat4 v_inv;

vec4 scene_ambient = vec4(0.2, 0.2, 0.2, 1.0);

struct material {
  vec4 ambient;
  vec4 diffuse;
  vec4 specular;
  float shininess;
};
material frontMaterial = material(
  vec4(0.2, 0.2, 0.2, 1.0),
  vec4(1.0, 0.8, 0.8, 1.0),
  vec4(1.0, 1.0, 1.0, 1.0),
  5.0
);
material backMaterial = material(
  vec4(0.2, 0.2, 0.2, 1.0),
  vec4(0.0, 0.0, 1.0, 1.0),
  vec4(1.0, 1.0, 1.0, 1.0),
  5.0
);

void main() {
  vec3 normalDirection = normalize(varyingNormalDirection);
  vec4 ambientColor = color;
  vec4 diffuseColor = color;
  vec4 specularColor = color;
  float shininess = 5.0;

  vec3 viewDirection = normalize(vec3(v_inv * vec4(0.0, 0.0, 0.0, 1.0) - position));
  vec3 lightDirection;
  float attenuation;

  vec3 ambientLighting = vec3(0.0,0.0,0.0);
  vec3 diffuseReflection = vec3(0.0,0.0,0.0);
  vec3 specularReflection = vec3(0.0,0.0,0.0);

  for(int i=0; i<gl_MaxLights; i++){
    if (0.0 == gl_LightSource[i].position.w) {
      // directional light?
      attenuation = 1.0; // no attenuation
      lightDirection = normalize(vec3(gl_LightSource[i].position));
    } else {
      // point light or spotlight (or other kind of light)
      vec3 positionToLightSource = vec3(gl_LightSource[i].position - position);
      float distance = length(positionToLightSource);
      lightDirection = normalize(positionToLightSource);
      attenuation = 1.0 / (gl_LightSource[i].constantAttenuation + gl_LightSource[i].linearAttenuation * distance + gl_LightSource[i].quadraticAttenuation * distance * distance);

      if (gl_LightSource[i].spotCutoff <= 90.0){
        // spotlight?
        float clampedCosine = max(0.0, dot(-lightDirection, gl_LightSource[i].spotDirection));
        if (clampedCosine < cos(radians(gl_LightSource[i].spotCutoff))) {
          // outside of spotlight cone?
          attenuation = 0.0;
        } else {
          attenuation = attenuation * pow(clampedCosine, gl_LightSource[i].spotExponent);
        }
      }
    }

    ambientLighting += vec3(scene_ambient) * vec3(ambientColor);

    diffuseReflection += attenuation * vec3(gl_LightSource[i].diffuse) * vec3(diffuseColor) * max(0.0, dot(normalDirection, lightDirection));

    if (dot(normalDirection, lightDirection) < 0.0) {
      // light source on the wrong side?
      specularReflection += vec3(0.0, 0.0, 0.0); // no specular reflection
    } else {
      // light source on the right side
      specularReflection += attenuation * vec3(gl_LightSource[i].specular) * vec3(specularColor) * pow(max(0.0, dot(reflect(-lightDirection, normalDirection), viewDirection)), shininess);
    }
  }

  gl_FragColor = 0.5*vec4(ambientLighting + diffuseReflection + specularReflection, 1.0) + 0.5*texture2D(tex, tex_coord);
  //gl_FragColor = vec4(ambientLighting + diffuseReflection + specularReflection, 1.0) * texture2D(tex, tex_coord);
  //gl_FragColor = vec4(ambientLighting + diffuseReflection + specularReflection, 1.0);
}
