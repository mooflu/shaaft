layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 color;
layout (location = 3) in vec2 uv;

out vec4 v_color;
out vec2 v_uv;

void main()
{
    v_uv = vertex.xy;
    v_color = color;
    gl_Position = vertex;
}
