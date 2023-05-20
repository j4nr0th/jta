#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal_in;

layout(location = 2) in vec4 color_in;
layout(location = 3) in mat4 model_transform;

layout(location = 0) out vec4 vtx_color;
//layout(location = 1) out vec4 vtx_normal;

layout(binding = 0) uniform UBO_struct
{
    mat4 view;
    mat4 proj;
};

void main()
{
    vec4 p = view * model_transform * vec4(position, 1.0);
    float k = length(p.xyz);
    if (k == 0.0f)
    {
        gl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        gl_Position = proj * p / k;
    }
    gl_Position.w = 1.0f;
    vtx_color = color_in;
}

