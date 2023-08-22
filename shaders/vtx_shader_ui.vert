#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 in_tex_coords;
layout(location = 2) in int in_tex_idx;
layout(location = 3) in vec4 color_in;

layout(location = 0) out vec4 vtx_color;
layout(location = 1) out flat int tex_idx;
layout(location = 2) out vec2 tex_coords;


layout( push_constant ) uniform UBO_struct
{
    vec2 scale;
    vec2 offset;
};

void main()
{
    gl_Position = vec4((position - offset) * scale, 0.0f, 1.0f);

    vtx_color = color_in;
    tex_coords = in_tex_coords;
    tex_idx = in_tex_idx;
}

