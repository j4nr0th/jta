#version 450

layout(location=0) out vec4 color_out;
layout(location = 0) in vec3 vtx_color;

void main()
{
    color_out = vec4(vtx_color, 1.0);
}