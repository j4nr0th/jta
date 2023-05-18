#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color_in;

layout(location = 0) out vec3 vtx_color;

layout(binding = 0) uniform UBO_struct
{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main()
{
    vec4 p = ubo.view * ubo.model * vec4(position, 1.0);
    float k = length(p.xyz);
    if (k == 0.0f)
    {
        ubo.projection * p / 1.0f;
    }
    else
    {
        gl_Position = ubo.projection * p / k;
    }
//    gl_Position.z /= gl_Position.z + 1.0f;
    gl_Position.w = 1;
    vtx_color = color_in;
}