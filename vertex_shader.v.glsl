attribute vec4 v_coord;
attribute vec3 v_normal;
attribute vec4 v_color;
attribute vec2 v_texture;

varying vec4 position;  // position of the vertex (and fragment) in world space
varying vec3 varyingNormalDirection;  // surface normal vector in world space
varying vec4 color;
varying vec2 tex_coord;

uniform mat4 m, v, p; // model, view, projection matrix
uniform mat3 m_3x3_inv_transp;

void main() {
  position = m * v_coord;
  varyingNormalDirection = normalize(m_3x3_inv_transp * v_normal);
  color = v_color;
  tex_coord = v_texture;

  mat4 mvp = p*v*m;
  gl_Position = mvp * v_coord;
}
