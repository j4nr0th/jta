#version 450

layout(location = 0) in vec4 vtx_color;
layout(location = 1) in vec4 vtx_normal;
layout(location = 2) in vec3 view_vector;

layout(location = 0) out vec4 color_out;

void main()
{
    color_out = vtx_color;
    color_out.rgb *= -dot(vtx_normal.xyz, view_vector.xyz);
}