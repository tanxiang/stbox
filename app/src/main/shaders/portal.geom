#version 450


layout(lines_adjacency)in;
layout(triangle_strip,max_vertices=6) out;

layout (binding = 0) uniform UBO
{
    mat4 MVP;
} ubo;
//layout (location = 0) in vec4 inColor[];

layout (location = 0) out vec4 outColor;


void main() {
    vec4 v0 = ubo.MVP*gl_in[0].gl_Position;
    vec4 v1 = ubo.MVP*gl_in[1].gl_Position;
    vec4 v2 = ubo.MVP*gl_in[2].gl_Position;
    vec4 v3 = ubo.MVP*gl_in[3].gl_Position;

    gl_Position = v0;
    outColor = vec4(0.0, 1.0, 0.0,1.0);
    EmitVertex();

    gl_Position = v1;
    outColor = vec4(1.0, 1.0, 0.0,1.0);
    EmitVertex();

    gl_Position = v2;
    outColor = vec4(0.0, 0.0, 0.0,1.0);
    EmitVertex();

    gl_Position = v3;
    outColor = vec4(1.0, 0.0, 0.0,1.0);
    EmitVertex();

    gl_Position = v0;
    outColor = vec4(1.0, 0.0, 1.0,1.0);
    EmitVertex();

    gl_Position = v1;
    outColor = vec4(1.0, 1.0, 0.0,1.0);
    EmitVertex();

    EndPrimitive();
}