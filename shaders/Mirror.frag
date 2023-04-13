#version 330


uniform vec3 u_cam_pos;

uniform samplerCube u_texture_cubemap;

in vec4 v_position;
in vec4 v_normal;
in vec4 v_tangent;

out vec4 out_color;

void main() {
  // YOUR CODE HERE
  vec4 n = normalize(v_normal);

  out_color = texture(u_texture_cubemap, -((u_cam_pos - v_position.xyz) - 2 * (dot(n.xyz, u_cam_pos - v_position.xyz)) * n.xyz));
  out_color.a = 1;
}
