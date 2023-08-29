#version 450

layout(location = 0) in vec4 vtx_color;
layout(location = 1) in flat int tex_idx;
layout(location = 2) in vec2 tex_coords;

layout(location = 0) out vec4 color_out;

//layout(binding = 0) uniform sampler2D texture_sampler;

void main()
{
    color_out = vtx_color;
//    vec4 tex_clr;
//    if (tex_idx > 0)
//    {
//        tex_clr = texture(texture_sampler, tex_coords);
//    }
//    else
//    {
//        tex_clr = vec4(1.0f);
//    }
//
//    color_out *= vec4(tex_clr.a);
}
