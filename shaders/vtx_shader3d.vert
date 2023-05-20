#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color_in;

layout(location = 0) out vec4 vtx_color;

layout(binding = 0) uniform UBO_struct
{
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;

void main()
{
    vec4 p = ubo.view * ubo.model * vec4(position, 1.0);
    float k = length(p.xyz);
    if (k == 0.0f)
    {
        gl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        gl_Position = ubo.proj * p / k;
    }
    gl_Position.w = 1.0f;
    vtx_color = color_in;
}

