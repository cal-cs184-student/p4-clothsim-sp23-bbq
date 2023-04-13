#version 330

// The camera's position in world-space
uniform vec3 u_cam_pos;

// Color
uniform vec4 u_color;

// Properties of the single point light
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

// We also get the uniform texture we want to use.
uniform sampler2D u_texture_1;

// These are the inputs which are the outputs of the vertex shader.
in vec4 v_position;
in vec4 v_normal;

// This is where the final pixel color is output.
// Here, we are only interested in the first 3 dimensions (xyz).
// The 4th entry in this vector is for "alpha blending" which we
// do not require you to know about. For now, just set the alpha
// to 1.
out vec4 out_color;

void main() {
  // YOUR CODE HERE
  float w_coordinate = 1.0;
  vec4 r = vec4(u_light_pos, w_coordinate) - v_position;
  float r_len = length(r);

  vec4 I = vec4(u_light_intensity, w_coordinate);
  // vec4 n_dot_l = dot(normalize(r), v_normal);
  // vec4 L_d = 1.0 * (I/(r_len * r_len)) * max(0.0, dot(normalize(r), v_normal));


  // (Placeholder code. You will want to replace it.)
  out_color = 1.0 * (I/(r_len * r_len)) * max(0.0, dot(normalize(r), v_normal));
  out_color.a = 1;
}
