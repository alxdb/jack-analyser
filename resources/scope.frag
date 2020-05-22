#version 410

out vec4 color;

uniform vec3 line_color = vec3(1, 1, 1);

void main() {
  color = vec4(line_color, 1);
  /* color = vec4(0.0, 0.0, 0.0, 1); */
}
