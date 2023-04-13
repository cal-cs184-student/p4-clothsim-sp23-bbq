#version 330

uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

uniform vec4 u_color;

uniform sampler2D u_texture_2;
uniform vec2 u_texture_2_size;

uniform float u_normal_scaling;
uniform float u_height_scaling;

in vec4 v_position;
in vec4 v_normal;
in vec4 v_tangent;
in vec2 v_uv;

out vec4 out_color;

float h(vec2 uv) {
  // You may want to use this helper function...
  return texture(u_texture_2, uv).r;
}

void main() {
  // YOUR CODE HERE
  float du = (h(vec2(1.0 + v_uv[0]/u_texture_2_size[0], v_uv[1])) - h(v_uv)) * u_normal_scaling * u_height_scaling;
  float dv = (h(vec2(v_uv[0], v_uv[1] + 1.0/u_texture_2_size[1])) - h(v_uv)) * u_normal_scaling * u_height_scaling;

  vec3 l = u_light_pos - v_position.xyz;
  vec3 v = normalize(u_cam_pos - v_position.xyz);
  vec3 b = cross(v_normal.xyz, v_tangent.xyz);
  mat3 tbn = mat3(v_tangent.xyz, b, v_normal.xyz);

  out_color = vec4(0.3, 0.3, 0.3, 0.3)+ vec4(u_color.xyz * (u_light_intensity / (dot(l, l))) * max(0, dot(normalize(normalize(tbn * vec3(-du, -dv, 1)).xyz), normalize(l))), 1.0)+ vec4(.5 * (u_light_intensity / (dot(l, l))) * pow(max(0, dot(normalize(normalize(tbn * vec3(-du, -dv, 1)).xyz), normalize((v + normalize(l)) / sqrt(dot((v + normalize(l)), (v + normalize(l))))))), 100), 1.0);
  out_color.a = 1;
}


