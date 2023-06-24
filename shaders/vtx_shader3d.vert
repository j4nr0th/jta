#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal_in;

layout(location = 2) in vec4 color_in;
layout(location = 3) in mat4 model_transform;

layout(location = 0) out vec4 vtx_color;
layout(location = 1) out vec4 vtx_normal;
layout(location = 2) out vec3 view_vector;

layout(binding = 0) uniform UBO_struct
{
    mat4 view;
    mat4 proj;
    vec3 view_dir;
};

void main()
{
    vec4 p = view * model_transform * vec4(position, 1.0f);
    gl_Position = proj * p;
    gl_Position /= p.z;

    vtx_color = color_in;
    vtx_normal = normalize(view * model_transform * vec4(normal_in, 1.0f));
    view_vector = normalize(view_dir);
}

