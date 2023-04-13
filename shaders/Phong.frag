#version 330

uniform vec4 u_color;
uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

in vec4 v_position;
in vec4 v_normal;
in vec2 v_uv;

out vec4 out_color;

void main() {
  // YOUR CODE HERE
  float w_coordinate = 1.0;
  vec4 r = vec4(u_light_pos, w_coordinate) - v_position;
  float r_len = length(r);
  vec4 I = vec4(u_light_intensity, w_coordinate);
  vec4 l = normalize(r);
  vec4 h = normalize(vec4(u_cam_pos, w_coordinate) - v_position);


  out_color = 0.1 * vec4(1.0, 2.0, 3.0, 4.0)
            + 0.2 * (I/(r_len * r_len)) * max(0.0, dot(l, v_normal));
            + 0.3 * (I / (r_len * r_len)) * pow(max(0.0, dot(normalize(l + h), v_normal)), 4);


  out_color.a = 1;

}

