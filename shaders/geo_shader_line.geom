#version 450
#define M_PI 3.1415926535897932384626433832795


layout(lines) in;
layout(triangle_strip, max_vertices = 256) out;

layout(location = 0) in VS_Out
{
    float x, y, z;
    float r_base;
    vec4 color_in;
} gs_in[];

layout(binding = 0) uniform UBO_struct
{
    mat4 proj;
    mat4 view;
    uint pts_per_side;
};

layout(location = 0) out vec4 color_vtx;

void main()
{
    //  Determine the direction vector
    vec4 v1 = vec4(gs_in[0].x, gs_in[0].y, gs_in[0].z, 1.0f);
    vec4 v2 = vec4(gs_in[1].x, gs_in[1].y, gs_in[1].z, 1.0f);


    //  Compute transformation
    vec4 d = v2 - v1;
    d /= length(d);
    float d_omega = M_PI / (float(pts_per_side) * 2.0f);
    vec2 d_flat = d.xy;
    d_flat /= length(d_flat);
    vec3 dr;

    //  Emit vertex that is meant to be on the v2 side of the cylinder
    color_vtx = gs_in[1].color_in;
    float co = cos(d_omega * 0);
    float so = sin(d_omega * 0);
    dr.x = d.z * co;
    dr.y = d_flat.x * so + d.y * co;
    dr.z = -d_flat.y * so + d.x * co;
    gl_Position = proj * view * (v2 + vec4(gs_in[1].r_base * dr, 1.0f));
    EmitVertex();

    //  Emit vertex that is meant to be on the v1 side of the cylinder
    color_vtx = gs_in[0].color_in;
    co = cos(d_omega * 1);
    so = sin(d_omega * 1);
    dr.x = d.z * co;
    dr.y = d_flat.x * so + d.y * co;
    dr.z = -d_flat.y * so + d.x * co;
    gl_Position = proj * view * (v1 + vec4(gs_in[0].r_base * dr, 1.0f));
    EmitVertex();


    for (uint i = 1; i < pts_per_side + 1; ++i)
    {
        //  Emit vertex that is meant to be on the v2 side of the cylinder
        color_vtx = gs_in[1].color_in;
        float co = cos(d_omega * float(i * 2));
        float so = sin(d_omega * float(i * 2));
        dr.x = d.z * co;
        dr.y = d_flat.x * so + d.y * co;
        dr.z = -d_flat.y * so + d.x * co;
        gl_Position = proj * view * (v2 + vec4(gs_in[1].r_base * dr, 1.0f));
        EmitVertex();
        EndPrimitive();

        //  Emit vertex that is meant to be on the v1 side of the cylinder
        color_vtx = gs_in[0].color_in;
        co = cos(d_omega * float(i * 2 + 1));
        so = sin(d_omega * float(i * 2 + 1));
        dr.x = d.z * co;
        dr.y = d_flat.x * so + d.y * co;
        dr.z = -d_flat.y * so + d.x * co;
        gl_Position = proj * view * (v1 + vec4(gs_in[0].r_base * dr, 1.0f));
        EmitVertex();
        EndPrimitive();
    }

}
